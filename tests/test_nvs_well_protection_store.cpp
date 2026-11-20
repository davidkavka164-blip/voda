#include "NvsWellProtectionStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

Preferences::Entry Preferences::entries[16] = {};
int32_t Preferences::writeLimit = -1;
size_t Preferences::failAtWriteCall = 0, Preferences::failBytes = 0, Preferences::writeCallCount = 0;

int main() {
    using namespace HWC;
    Preferences::clearForTest();
    NvsWellProtectionStore store;
    assert(store.begin());
    assert(!store.hasRecord());

    WellProtectionConfig config = emptyWellProtectionConfig();
    config.transferMinimumLevelCm = 60.0f;
    config.criticalTripLevelCm = 35.0f;
    config.recoveryLevelCm = 55.0f;
    updateWellProtectionConfigChecksum(config);
    assert(isWellProtectionConfigValid(config, 800.0f));
    assert(!isWellProtectionConfigValid(config, 50.0f));
    assert(store.save(config, false, true));
    assert(store.hasRecord());

    WellProtectionConfig loaded = emptyWellProtectionConfig();
    bool latch = true;
    bool safetyValid = false;
    assert(store.load(loaded, latch, safetyValid));
    assert(loaded.transferMinimumLevelCm == 60.0f);
    assert(!latch && safetyValid);
    assert(store.saveCriticalLatch(true));
    assert(store.load(loaded, latch, safetyValid) && latch && safetyValid);
    assert(store.saveCriticalLatch(false));
    assert(store.load(loaded, latch, safetyValid) && !latch && safetyValid);

    // Corrupt the persisted state record in-place.
    Preferences::entries[0].bytes[0] ^= 0xFFU;
    assert(!store.load(loaded, latch, safetyValid));
    assert(!store.save(config, false, true)); // no silent overwrite of a corrupt safety record
    store.end();

    // A partial update corrupts only the inactive slot; strict policy rejects
    // ambiguous state while the previous slot bytes remain untouched.
    Preferences::clearForTest();
    NvsWellProtectionStore interruptedStore;
    assert(interruptedStore.begin());
    assert(interruptedStore.save(config, true, true));
    uint8_t previousSlot[256] = {};
    memcpy(previousSlot, Preferences::entries[0].bytes, Preferences::entries[0].length);
    const size_t previousLength = Preferences::entries[0].length;
    Preferences::simulatePartialWriteAfter(3);
    assert(!interruptedStore.saveCriticalLatch(false));
    Preferences::clearWriteFailure();
    WellProtectionConfig afterCut = emptyWellProtectionConfig();
    bool afterCutLatch = false, afterCutValid = true;
    assert(interruptedStore.hasRecord());
    assert(!interruptedStore.load(afterCut, afterCutLatch, afterCutValid));
    assert(Preferences::entries[0].length == previousLength);
    assert(memcmp(previousSlot, Preferences::entries[0].bytes, previousLength) == 0);
    interruptedStore.end();

    // Reusing an older, already-valid slot must leave the newest slot intact
    // if power is interrupted during the replacement write.
    Preferences::clearForTest();
    NvsWellProtectionStore reuseStore;
    assert(reuseStore.begin());
    assert(reuseStore.save(config, true, true));
    assert(reuseStore.save(config, false, true));
    assert(reuseStore.save(config, true, true));
    Preferences::Entry newestWell = {};
    for (size_t i = 0; i < 16; ++i)
        if (Preferences::entries[i].used && strcmp(Preferences::entries[i].key, "slot0") == 0) newestWell = Preferences::entries[i];
    assert(newestWell.used);
    // slot0 contains the newest generation; interrupt replacement of slot1.
    Preferences::simulatePartialWriteAfter(5);
    assert(!reuseStore.saveCriticalLatch(false));
    Preferences::clearWriteFailure();
    bool recoveredLatch = false, recoveredValid = false;
    WellProtectionConfig recoveredConfig = emptyWellProtectionConfig();
    assert(!reuseStore.load(recoveredConfig, recoveredLatch, recoveredValid));
    bool newestStillPresent = false;
    for (size_t i = 0; i < 16; ++i) {
        const Preferences::Entry& e = Preferences::entries[i];
        if (e.used && strcmp(e.key, "slot0") == 0 && e.length == newestWell.length &&
            memcmp(e.bytes, newestWell.bytes, newestWell.length) == 0) newestStillPresent = true;
    }
    assert(newestStillPresent);
    reuseStore.end();

    // Legacy state migrates to slot0 only after successful read-back; legacy
    // bytes remain available for rollback/recovery.
    Preferences::clearForTest();
    WellProtectionRecord legacy = {};
    legacy.magic = WELL_PROTECTION_RECORD_MAGIC;
    legacy.version = WELL_PROTECTION_RECORD_VERSION;
    legacy.config = config;
    legacy.criticalLatch = 1;
    legacy.safetyStateValid = 0; // invalid safety state must remain invalid
    legacy.checksum = wellProtectionRecordChecksum(legacy);
    Preferences legacyPrefs;
    assert(legacyPrefs.begin("hwc_protect", false));
    assert(legacyPrefs.putBytes("state", &legacy, sizeof(legacy)) == sizeof(legacy));
    legacyPrefs.end();
    NvsWellProtectionStore migrated;
    assert(migrated.begin() && migrated.hasRecord());
    bool migratedLatch = false, migratedValid = true;
    WellProtectionConfig migratedConfig = emptyWellProtectionConfig();
    assert(migrated.load(migratedConfig, migratedLatch, migratedValid));
    assert(migratedLatch && !migratedValid);
    assert(Preferences::entries[1].used || Preferences::entries[0].used);
    assert(migrated.hasRecord());
    assert(migrated.saveCriticalLatch(true));
    assert(migrated.load(migratedConfig, migratedLatch, migratedValid));
    assert(migratedLatch && !migratedValid); // invalid safety state is never upgraded implicitly
    migrated.end();

    // Corrupt legacy state is present, not a fresh installation, and cannot be overwritten.
    Preferences::clearForTest();
    legacy.checksum ^= 1U;
    Preferences badLegacyPrefs;
    assert(badLegacyPrefs.begin("hwc_protect", false));
    assert(badLegacyPrefs.putBytes("state", &legacy, sizeof(legacy)) == sizeof(legacy));
    badLegacyPrefs.end();
    NvsWellProtectionStore rejectedLegacy;
    assert(rejectedLegacy.begin() && rejectedLegacy.hasRecord());
    assert(!rejectedLegacy.load(migratedConfig, migratedLatch, migratedValid));
    assert(!rejectedLegacy.save(config, false, true));
    rejectedLegacy.end();

    NvsWellProtectionStore upperStore("hwc_upperprot");
    assert(upperStore.begin());
       puts("PASS: NvsWellProtectionStore native stub tests");
    return 0;
}
