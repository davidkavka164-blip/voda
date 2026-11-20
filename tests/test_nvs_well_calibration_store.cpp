#include "NvsWellCalibrationStore.h"
#include <Preferences.h>
#include <assert.h>
#include <stdio.h>

bool Preferences::present = false;
bool Preferences::ignoreNextWrite = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};

int main() {
    using namespace HWC;
    Preferences::clearForTest();
    const WellGeometryConfig geometry = defaultWellGeometryConfig();
    NvsWellCalibrationStore store;
    WellCalibration out = emptyWellCalibration();
    const CalibrationPoint first[] = {{100.0f, 500.0f}, {200.0f, 1200.0f}, {300.0f, 2000.0f}};

    assert(store.mergeAndSave(geometry, first, 3, true, out) == WELL_CAL_SAVE_STORE_NOT_OPEN);
    assert(store.begin());
    assert(store.mergeAndSave(geometry, first, 3, true, out) == WELL_CAL_SAVE_OK);
    assert(out.pointCount == 3 && out.absoluteAnchorKnown == 1);
    assert(wellCalibrationChecksumMatches(out));

    // New store instance simulates re-opening NVS after a reboot.
    store.end();
    NvsWellCalibrationStore reloadedStore;
    assert(reloadedStore.begin());
    WellCalibration loaded = emptyWellCalibration();
    assert(reloadedStore.load(loaded));
    assert(loaded.pointCount == 3 && loaded.points[1].volumeLiters == 1200.0f);

    const CalibrationPoint local[] = {{150.0f, 850.0f}, {250.0f, 1600.0f}};
    assert(reloadedStore.mergeAndSave(geometry, local, 2, true, out) == WELL_CAL_SAVE_OK);
    assert(out.pointCount == 4);
    assert(out.points[0].levelCm == 100.0f && out.points[0].volumeLiters == 500.0f);
    assert(out.points[1].levelCm == 150.0f && out.points[1].volumeLiters == 850.0f);
    assert(out.points[2].levelCm == 250.0f && out.points[2].volumeLiters == 1600.0f);
    assert(out.points[3].levelCm == 300.0f && out.points[3].volumeLiters == 2000.0f);

    // A backend may report a complete write while retaining the previous blob.
    // The store must reject this false success and preserve the old calibration.
    const CalibrationPoint newer[] = {{100.0f, 510.0f}, {200.0f, 1220.0f}, {300.0f, 2050.0f}};
    Preferences::ignoreNextWrite = true;
    assert(reloadedStore.mergeAndSave(geometry, newer, 3, true, out) == WELL_CAL_SAVE_WRITE_FAILED);
    WellCalibration afterIgnoredWrite = emptyWellCalibration();
    assert(reloadedStore.load(afterIgnoredWrite));
    assert(afterIgnoredWrite.points[0].volumeLiters == 500.0f);

    // Corrupt stored bytes: do not silently replace the existing record.
    Preferences::storage[0] ^= 0xFFU;
    assert(reloadedStore.mergeAndSave(geometry, local, 2, true, out) == WELL_CAL_SAVE_EXISTING_DATA_CORRUPT);
    puts("PASS: NvsWellCalibrationStore native stub tests");
    return 0;
}
