#include "WellCalibrationEditor.h"
#include <assert.h>
#include <stdio.h>

int main() {
    using namespace HWC;
    const WellGeometryConfig geometry = defaultWellGeometryConfig();
    WellCalibration existing = emptyWellCalibration();
    const CalibrationPoint oldPoints[] = {{50.0f, 200.0f}, {100.0f, 500.0f},
                                          {200.0f, 1200.0f}, {300.0f, 2000.0f},
                                          {400.0f, 2900.0f}};
    existing.pointCount = 5;
    for (size_t i = 0; i < 5; ++i) existing.points[i] = oldPoints[i];
    existing.absoluteAnchorKnown = 1;
    updateWellCalibrationChecksum(existing);

    // Recalibrate a local section: preserve points outside [100, 300] cm.
    const CalibrationPoint local[] = {{100.0f, 520.0f}, {150.0f, 850.0f},
                                      {250.0f, 1600.0f}, {300.0f, 2050.0f}};
    WellCalibration merged = emptyWellCalibration();
    assert(mergeWellCalibration(geometry, existing, local, 4, true, merged) == WELL_CAL_EDIT_OK);
    assert(merged.pointCount == 6);
    assert(merged.points[0].levelCm == 50.0f && merged.points[0].volumeLiters == 200.0f);
    assert(merged.points[1].levelCm == 100.0f && merged.points[1].volumeLiters == 520.0f);
    assert(merged.points[2].levelCm == 150.0f && merged.points[2].volumeLiters == 850.0f);
    assert(merged.points[3].levelCm == 250.0f && merged.points[3].volumeLiters == 1600.0f);
    assert(merged.points[4].levelCm == 300.0f && merged.points[4].volumeLiters == 2050.0f);
    assert(merged.points[5].levelCm == 400.0f && merged.points[5].volumeLiters == 2900.0f);
    assert(merged.absoluteAnchorKnown == 1 && wellCalibrationChecksumMatches(merged));
    // Inputs remain unchanged.
    assert(existing.points[1].volumeLiters == 500.0f);

    // A new point set that conflicts with the preserved left point must be rejected.
    const CalibrationPoint conflict[] = {{100.0f, 100.0f}, {150.0f, 300.0f}};
    WellCalibration untouched = merged;
    assert(mergeWellCalibration(geometry, existing, conflict, 2, true, untouched) == WELL_CAL_EDIT_VOLUME_CONFLICT);
    assert(untouched.points[1].volumeLiters == merged.points[1].volumeLiters);

    // Unknown reference cannot be mixed into a previously absolute curve.
    assert(mergeWellCalibration(geometry, existing, local, 4, false, untouched) == WELL_CAL_EDIT_ANCHOR_CONFLICT);

    const CalibrationPoint tooDeep[] = {{790.0f, 100.0f}, {810.0f, 200.0f}};
    assert(mergeWellCalibration(geometry, existing, tooDeep, 2, true, untouched) == WELL_CAL_EDIT_POINT_OUTSIDE_DEPTH);

    puts("PASS: WellCalibrationEditor native tests");
    return 0;
}
