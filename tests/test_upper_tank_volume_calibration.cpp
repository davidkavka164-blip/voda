#include "UpperTankVolumeCalibration.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
using namespace HWC;
int main() {
    UpperTankVolumeCalibration c = emptyUpperTankVolumeCalibration();
    assert(isUpperTankVolumeCalibrationValid(c));
    assert(estimateUpperTankVolume(50.0f, c).status == UPPER_VOLUME_NOT_CONFIGURED);

    UpperTankVolumeCalibration next = emptyUpperTankVolumeCalibration();
    assert(addUpperTankVolumeCalibrationPoint(c, 100.0f, 5000.0f, false, next) == UPPER_VOLUME_EDIT_NEEDS_ABSOLUTE_CONFIRMATION);
    assert(addUpperTankVolumeCalibrationPoint(c, 100.0f, 5000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    assert(next.pointCount == 1 && next.absoluteAnchorKnown);
    assert(estimateUpperTankVolume(100.0f, next).status == UPPER_VOLUME_EXACT);
    assert(estimateUpperTankVolume(99.0f, next).status == UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE);

    c = next;
    assert(addUpperTankVolumeCalibrationPoint(c, 50.0f, 2500.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    c = next;
    assert(addUpperTankVolumeCalibrationPoint(c, 150.0f, 8000.0f, true, next) == UPPER_VOLUME_EDIT_OK);
    c = next;
    UpperTankVolumeResult mid = estimateUpperTankVolume(75.0f, c);
    assert(mid.status == UPPER_VOLUME_INTERPOLATED && fabsf(mid.liters - 3750.0f) < 0.01f);
    assert(estimateUpperTankVolume(151.0f, c).status == UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE);

    CalibrationPoint after = {};
    assert(deriveUpperTankPostTransferPoint(c, 100.0f, 80.0f, 1200.0f, after));
    assert(fabsf(after.volumeLiters - 3800.0f) < 0.01f);
    assert(!deriveUpperTankPostTransferPoint(c, 40.0f, 30.0f, 100.0f, after));
    assert(!deriveUpperTankPostTransferPoint(c, 100.0f, 80.0f, 6000.0f, after));

    UpperTankVolumeCalibration reference = emptyUpperTankVolumeCalibration();
    assert(setUpperTankTransferReference(c, 100.0f, reference));
    assert(reference.transferReferenceValid);
    UpperTankVolumeCalibration manualDuringReference = emptyUpperTankVolumeCalibration();
    assert(addUpperTankVolumeCalibrationPoint(reference, 90.0f, 4400.0f, true,
                                              manualDuringReference) == UPPER_VOLUME_EDIT_CONFLICT);
    assert(reference.transferReferenceValid);
    assert(manualDuringReference.pointCount == 0);
    UpperTankVolumeCalibration cancelled = emptyUpperTankVolumeCalibration();
    assert(clearUpperTankTransferReference(reference, cancelled));
    assert(!cancelled.transferReferenceValid);
    assert(addUpperTankPostTransferPoint(reference, 80.0f, 1200.0f, next) == UPPER_VOLUME_EDIT_OK);
    assert(!next.transferReferenceValid);
    assert(next.pointCount == 4);
    assert(next.points[1].levelCm == 80.0f && next.points[1].volumeLiters == 3800.0f);
    UpperTankVolumeCalibration corrupt = next;
    corrupt.points[1].volumeLiters += 100.0f;
    assert(!upperTankVolumeCalibrationChecksumMatches(corrupt));
    assert(estimateUpperTankVolume(80.0f, corrupt).status == UPPER_VOLUME_INVALID_CALIBRATION);

    assert(addUpperTankVolumeCalibrationPoint(c, 80.0f, 100.0f, true, next) == UPPER_VOLUME_EDIT_CONFLICT);
    assert(!setUpperTankTransferReference(c, 40.0f, reference));
    assert(addUpperTankPostTransferPoint(c, 80.0f, 100.0f, next) == UPPER_VOLUME_EDIT_INVALID_POINT);
    // Start the first transfer with the exact sample used for the first point.
    UpperTankVolumeCalibration first = emptyUpperTankVolumeCalibration();
    assert(addUpperTankVolumeCalibrationPoint(first, 123.456f, 4200.0f, true, first) == UPPER_VOLUME_EDIT_OK);
    assert(setUpperTankTransferReference(first, 123.456f, first));
    assert(first.transferReferenceValid && first.transferReferenceVolumeLiters == 4200.0f);
    UpperTankVolumeCalibration afterFirstTransfer = emptyUpperTankVolumeCalibration();
    assert(addUpperTankPostTransferPoint(first, 103.123f, 800.0f, afterFirstTransfer) == UPPER_VOLUME_EDIT_OK);
    assert(!afterFirstTransfer.transferReferenceValid && afterFirstTransfer.pointCount == 2);
    const UpperTankVolumeResult between = estimateUpperTankVolume(113.2895f, afterFirstTransfer);
    assert(between.status == UPPER_VOLUME_INTERPOLATED);
    assert(between.liters > 3799.9f && between.liters < 3800.1f);

    puts("PASS: upper tank absolute volume calibration, interpolation, transfer-derived points and validation");
}
