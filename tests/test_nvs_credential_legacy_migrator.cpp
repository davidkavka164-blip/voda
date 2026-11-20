#include "CredentialLegacyMigrator.h"
#include "NvsCredentialBackend.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

std::map<std::string, std::vector<uint8_t> > Preferences::values_;

static HWC::Credentials legacyCredentials() {
    HWC::Credentials c;
    HWC::CredentialStore::clear(c);
    strcpy(c.apName, "HomeWater-ABCD");
    strcpy(c.apPassword, "H12345678");
    strcpy(c.adminPassword, "HWC-A1B2C3D4E5F6");
    return c;
}

int main() {
    Preferences::resetForTest();
    HWC::Credentials legacy = legacyCredentials();
    HWC::Credentials active;
    HWC::CredentialStore::clear(active);

    // First boot: empty target is migrated through the actual Preferences backend.
    HWC::NvsCredentialBackend backend("hwcCred");
    assert(backend.begin());
    HWC::CredentialStore store(backend);
    assert(store.load(active) == HWC::CREDENTIAL_EMPTY);
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(store, legacy, active) == HWC::CREDENTIAL_MIGRATION_DONE);
    assert(HWC::CredentialStore::validate(active));
    assert(strcmp(active.apName, legacy.apName) == 0);
    assert(Preferences::existsForTest("hwcCred", "cred_active"));
    backend.end();

    // Reboot: a fresh backend instance must read the committed slot and selector.
    HWC::NvsCredentialBackend rebootBackend("hwcCred");
    assert(rebootBackend.begin());
    HWC::CredentialStore rebootStore(rebootBackend);
    HWC::CredentialStore::clear(active);
    assert(rebootStore.load(active) == HWC::CREDENTIAL_VALID);
    assert(strcmp(active.apPassword, legacy.apPassword) == 0);

    // Corruption of selected slot must fail closed; old legacy values cannot repair it.
    assert(Preferences::eraseForTest("hwcCred", "c0_admin"));
    HWC::CredentialStore::clear(active);
    assert(rebootStore.load(active) == HWC::CREDENTIAL_CORRUPT);
    assert(HWC::CredentialLegacyMigrator::loadOrMigrate(rebootStore, legacy, active) ==
           HWC::CREDENTIAL_MIGRATION_TARGET_CORRUPT);
    assert(rebootStore.load(active) == HWC::CREDENTIAL_CORRUPT);
    rebootBackend.end();

    HWC::CredentialStore::clear(legacy);
    HWC::CredentialStore::clear(active);
    puts("PASS: CredentialLegacyMigrator + Preferences backend end-to-end boot/reboot/corruption test");
}
