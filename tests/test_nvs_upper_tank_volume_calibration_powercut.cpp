#include "NvsUpperTankVolumeCalibrationStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>

bool Preferences::present = false;
size_t Preferences::length = 0U;
uint8_t Preferences::storage[512] = {0};
Preferences::WriteFault Preferences::writeFault = Preferences::WRITE_OK;

static HWC::UpperTankVolumeCalibration makeBaseCalibration() {
    using namespace HWC;
    UpperTankVolumeCalibration c = emptyUpperTankVolumeCalibration();
    UpperTankVolumeCalibration next = emptyUpperTankVolumeCalibration();
    assert(addUpperTankVolumeCalibrationPoint(c, 30.0f, 1000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    c = next;
    assert(addUpperTankVolumeCalibrationPoint(c, 90.0f, 4000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    return next;
}

static HWC::UpperTankVolumeCalibration loadAfterRestart() {
    HWC::NvsUpperTankVolumeCalibrationStore store;
    HWC::UpperTankVolumeCalibration loaded = HWC::emptyUpperTankVolumeCalibration();
    assert(store.begin());
    assert(store.load(loaded));
    store.end();
    return loaded;
}

int main() {
    using namespace HWC;
    Preferences::clearForTest();
    const UpperTankVolumeCalibration base = makeBaseCalibration();
    UpperTankVolumeCalibration reference = emptyUpperTankVolumeCalibration();
    assert(setUpperTankTransferReference(base, 60.0f, reference));

    // Power loss before the reference commit leaves the previous calibration.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        assert(store.save(base));
        Preferences::writeFault = Preferences::WRITE_INTERRUPTED_BEFORE_COMMIT;
        assert(!store.save(reference));
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    UpperTankVolumeCalibration afterInterruptedReference = loadAfterRestart();
    assert(!afterInterruptedReference.transferReferenceValid);
    assert(afterInterruptedReference.pointCount == base.pointCount);

    // Once committed, a reference survives restart and remains available to finish.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        assert(store.save(reference));
        store.end();
    }
    UpperTankVolumeCalibration afterReferenceRestart = loadAfterRestart();
    assert(afterReferenceRestart.transferReferenceValid);
    assert(afterReferenceRestart.transferReferenceLevelCm == reference.transferReferenceLevelCm);
    assert(afterReferenceRestart.transferReferenceVolumeLiters == reference.transferReferenceVolumeLiters);

    // An interrupted final-point write must leave the committed reference intact,
    // so the operator can retry rather than losing the mathematical starting point.
    UpperTankVolumeCalibration completed = emptyUpperTankVolumeCalibration();
    assert(addUpperTankPostTransferPoint(afterReferenceRestart, 45.0f, 1200.0f, completed) == UPPER_VOLUME_EDIT_OK);
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::WRITE_INTERRUPTED_BEFORE_COMMIT;
        assert(!store.save(completed));
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    UpperTankVolumeCalibration afterInterruptedCompletion = loadAfterRestart();
    assert(afterInterruptedCompletion.transferReferenceValid);
    assert(afterInterruptedCompletion.transferReferenceLevelCm == reference.transferReferenceLevelCm);

    // If NVS committed the final record but the API result was lost, runtime remains
    // blocked; after restart the complete new record is loaded exactly once.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::WRITE_COMMITTED_BUT_REPORT_FAILURE;
        assert(!store.save(completed));
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    UpperTankVolumeCalibration afterCommittedButUnreported = loadAfterRestart();
    assert(!afterCommittedButUnreported.transferReferenceValid);
    assert(afterCommittedButUnreported.pointCount == completed.pointCount);
    assert(afterCommittedButUnreported.points[2].levelCm == completed.points[2].levelCm);
    assert(afterCommittedButUnreported.points[2].volumeLiters == completed.points[2].volumeLiters);

    // Recovery refuses to erase a valid record, including when a read fails.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::READ_FAILURE;
        assert(!store.discardCorruptRecord());
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    assert(Preferences::present);
    // Corrupt the record, then prove that a read failure still cannot authorize deletion.
    Preferences::storage[0] ^= 0x80;
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::READ_FAILURE;
        assert(!store.discardCorruptRecord());
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    assert(Preferences::present);
    // A failed removal leaves the corrupt key in place and recovery remains blocked.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::REMOVE_INTERRUPTED_BEFORE_COMMIT;
        assert(!store.discardCorruptRecord());
        store.end();
    }
    assert(Preferences::present);
    // If removal commits but the API reports failure, reboot sees no key; the
    // calibration is then explicitly unconfigured and requires a new absolute anchor.
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        Preferences::writeFault = Preferences::REMOVE_COMMITTED_BUT_REPORT_FAILURE;
        assert(!store.discardCorruptRecord());
        store.end();
    }
    Preferences::writeFault = Preferences::WRITE_OK;
    assert(!Preferences::present);
    {
        NvsUpperTankVolumeCalibrationStore store;
        assert(store.begin());
        assert(!store.hasRecord());
        UpperTankVolumeCalibration empty = emptyUpperTankVolumeCalibration();
        assert(!store.load(empty));
        store.end();
    }

    puts("PASS: upper volume calibration NVS write/remove power-loss and restart scenarios");
    return 0;
}
