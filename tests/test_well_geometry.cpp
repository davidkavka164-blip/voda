#include "WellGeometry.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

static bool near(float a, float b, float e = 0.1f) { return fabsf(a - b) <= e; }

int main() {
    using namespace HWC;
    WellGeometryConfig g = defaultWellGeometryConfig();
    assert(isWellGeometryConfigValid(g));
    assert(wellGeometryChecksumMatches(g));
    assert(g.internalDiameterCm == 80.0f && g.usableDepthCm == 800.0f);

    float liters = 0.0f;
    assert(estimateGeometryVolumeLiters(800.0f, g, liters));
    assert(near(liters, 4021.238f, 0.2f));
    assert(estimateGeometryVolumeLiters(100.0f, g, liters));
    assert(near(liters, 502.654f, 0.1f));
    assert(!estimateGeometryVolumeLiters(801.0f, g, liters));

    WellCalibration empty = emptyWellCalibration();
    assert(isWellCalibrationValid(empty) && wellCalibrationChecksumMatches(empty));
    WellVolumeResult r = estimateWellVolume(g, 400.0f, empty);
    assert(r.status == WELL_VOLUME_GEOMETRY_ESTIMATE && !r.absoluteVolumeKnown);
    assert(near(r.liters, 2010.619f, 0.2f));

    const CalibrationPoint measured[] = {{100.0f, 600.0f}, {200.0f, 1200.0f}, {300.0f, 1900.0f}};
    WellCalibration calibration = emptyWellCalibration();
    calibration.pointCount = 3;
    calibration.absoluteAnchorKnown = 1;
    for (size_t i = 0; i < 3; ++i) calibration.points[i] = measured[i];
    updateWellCalibrationChecksum(calibration);
    assert(isWellCalibrationValid(calibration) && wellCalibrationChecksumMatches(calibration));
    r = estimateWellVolume(g, 150.0f, calibration);
    assert(r.status == WELL_VOLUME_CALIBRATED_INTERPOLATED && r.absoluteVolumeKnown);
    assert(near(r.liters, 900.0f));
    const CalibrationPoint lowMeasured[] = {{100.0f, 400.0f}, {200.0f, 700.0f}};
    r = estimateWellVolume(g, 150.0f, lowMeasured, 2, true);
    assert(r.status == WELL_VOLUME_CALIBRATED_INTERPOLATED);
    assert(near(r.liters, 754.0f, 1.0f)); // geometry minimum, not 550 L
    r = estimateWellVolume(g, 100.0f, calibration);
    assert(r.status == WELL_VOLUME_CALIBRATED_EXACT && near(r.liters, 600.0f));
    r = estimateWellVolume(g, 50.0f, calibration);
    assert(r.status == WELL_VOLUME_GEOMETRY_ESTIMATE && r.absoluteVolumeKnown);
    // The positive cavity offset at the nearest calibrated boundary is retained.
    assert(near(r.liters, 348.673f, 0.2f));
    // At the calibrated lower boundary, measured volume may only add cavities.
    r = estimateWellVolume(g, 100.0f, calibration);
    assert(r.status == WELL_VOLUME_CALIBRATED_EXACT && near(r.liters, 600.0f));

    // Editable geometry changes the estimate but does not mutate measured points.
    WellGeometryConfig wider = g;
    wider.internalDiameterCm = 100.0f;
    updateWellGeometryChecksum(wider);
    assert(isWellGeometryConfigValid(wider) && wellGeometryChecksumMatches(wider));
    r = estimateWellVolume(wider, 150.0f, calibration);
    assert(r.status == WELL_VOLUME_CALIBRATED_INTERPOLATED && near(r.liters, 1178.097f, 0.5f));
    r = estimateWellVolume(wider, 400.0f, calibration);
    assert(r.status == WELL_VOLUME_GEOMETRY_ESTIMATE);
    assert(near(r.liters, 3141.593f, 0.5f));

    WellCalibration corrupted = calibration;
    corrupted.points[1].volumeLiters = 50.0f;
    assert(!wellCalibrationChecksumMatches(corrupted));
    corrupted = calibration;
    corrupted.pointCount = 1;
    updateWellCalibrationChecksum(corrupted);
    assert(!isWellCalibrationValid(corrupted));

    wider.internalDiameterCm = 0.0f;
    updateWellGeometryChecksum(wider);
    assert(!isWellGeometryConfigValid(wider));
    g.checksum ^= 0x01U;
    assert(!wellGeometryChecksumMatches(g));

    puts("PASS: WellGeometry native tests");
    return 0;
}
