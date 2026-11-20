#include "NvsUpperTankVolumeCalibrationStore.h"
#include <assert.h>
#include <stdio.h>
size_t Preferences::length=0; bool Preferences::present=false; unsigned char Preferences::storage[4096]={0}; char Preferences::lastNamespace[32]={0};
int main() {
    using namespace HWC;
    Preferences::clearForTest();
    NvsUpperTankVolumeCalibrationStore store;
    assert(store.begin());
    assert(!store.hasRecord());

    UpperTankVolumeCalibration c = emptyUpperTankVolumeCalibration();
    UpperTankVolumeCalibration next = emptyUpperTankVolumeCalibration();
    assert(addUpperTankVolumeCalibrationPoint(c, 30.0f, 1000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    c = next;
    assert(addUpperTankVolumeCalibrationPoint(c, 90.0f, 4000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    c = next;

    // Persist a transfer reference and verify all reference fields after readback.
    UpperTankVolumeCalibration referenced = emptyUpperTankVolumeCalibration();
    assert(setUpperTankTransferReference(c, 60.0f, referenced));
    assert(referenced.transferReferenceValid);
    assert(store.save(referenced));
    UpperTankVolumeCalibration got = emptyUpperTankVolumeCalibration();
    assert(store.load(got));
    assert(got.checksum == referenced.checksum && got.pointCount == 2);
    assert(got.absoluteAnchorKnown == referenced.absoluteAnchorKnown);
    assert(got.transferReferenceValid == referenced.transferReferenceValid);
    assert(got.transferReferenceLevelCm == referenced.transferReferenceLevelCm);
    assert(got.transferReferenceVolumeLiters == referenced.transferReferenceVolumeLiters);
    for (size_t i = 0; i < got.pointCount; ++i) {
        assert(got.points[i].levelCm == referenced.points[i].levelCm);
        assert(got.points[i].volumeLiters == referenced.points[i].volumeLiters);
    }

    // A failed validation must not replace the previously saved reference.
    UpperTankVolumeCalibration invalid = referenced;
    invalid.transferReferenceLevelCm += 1.0f; // checksum intentionally stale
    assert(!store.save(invalid));
    UpperTankVolumeCalibration afterRejectedSave = emptyUpperTankVolumeCalibration();
    assert(store.load(afterRejectedSave));
    assert(afterRejectedSave.checksum == referenced.checksum);
    assert(afterRejectedSave.transferReferenceValid);
    assert(afterRejectedSave.transferReferenceLevelCm == referenced.transferReferenceLevelCm);

    // A corrupt stored record is never loaded or overwritten by an ordinary save.
    Preferences::storage[0] ^= 0x80;
    UpperTankVolumeCalibration sentinel = emptyUpperTankVolumeCalibration();
    sentinel.pointCount = 7;
    assert(!store.load(sentinel));
    assert(sentinel.pointCount == 7); // failed load leaves caller's object untouched
    assert(!store.save(referenced));
    // Explicit recovery may discard a corrupt record, but never a valid one.
    assert(store.discardCorruptRecord());
    assert(!store.hasRecord());
    assert(!store.discardCorruptRecord());
    assert(store.save(referenced));
    assert(!store.discardCorruptRecord());
    UpperTankVolumeCalibration preserved = emptyUpperTankVolumeCalibration();
    assert(store.load(preserved));
    assert(preserved.transferReferenceValid);
    puts("PASS: upper tank volume calibration persistence, readback, corrupt-record recovery and valid-record protection");
}
