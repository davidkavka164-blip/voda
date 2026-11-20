#ifndef HWC_UPPER_TANK_VOLUME_CALIBRATION_H
#define HWC_UPPER_TANK_VOLUME_CALIBRATION_H

#include <stddef.h>
#include <stdint.h>
#include "WaterMath.h"

namespace HWC {

static const uint32_t UPPER_VOLUME_CAL_MAGIC = 0x48555643UL; // "HUVC"
static const uint16_t UPPER_VOLUME_CAL_VERSION = 1;
static const size_t MAX_UPPER_VOLUME_CAL_POINTS = 32;

struct UpperTankVolumeCalibration {
    uint32_t magic;
    uint16_t version;
    uint16_t pointCount;
    uint8_t absoluteAnchorKnown;
    uint8_t transferReferenceValid;
    uint8_t reserved[2];
    float transferReferenceLevelCm;
    float transferReferenceVolumeLiters;
    CalibrationPoint points[MAX_UPPER_VOLUME_CAL_POINTS];
    uint32_t checksum;
};

enum UpperTankVolumeStatus {
    UPPER_VOLUME_NOT_CONFIGURED,
    UPPER_VOLUME_EXACT,
    UPPER_VOLUME_INTERPOLATED,
    UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE,
    UPPER_VOLUME_INVALID_LEVEL,
    UPPER_VOLUME_INVALID_CALIBRATION,
    UPPER_VOLUME_ABSOLUTE_ANCHOR_UNKNOWN
};

struct UpperTankVolumeResult {
    UpperTankVolumeStatus status;
    float liters;
    bool absoluteVolumeKnown;
};

enum UpperTankVolumeEditStatus {
    UPPER_VOLUME_EDIT_OK,
    UPPER_VOLUME_EDIT_INVALID_EXISTING,
    UPPER_VOLUME_EDIT_INVALID_POINT,
    UPPER_VOLUME_EDIT_NEEDS_ABSOLUTE_CONFIRMATION,
    UPPER_VOLUME_EDIT_CONFLICT,
    UPPER_VOLUME_EDIT_TOO_MANY_POINTS
};

UpperTankVolumeCalibration emptyUpperTankVolumeCalibration();
bool isUpperTankVolumeCalibrationValid(const UpperTankVolumeCalibration& calibration);
uint32_t upperTankVolumeCalibrationChecksum(const UpperTankVolumeCalibration& calibration);
bool upperTankVolumeCalibrationChecksumMatches(const UpperTankVolumeCalibration& calibration);
void updateUpperTankVolumeCalibrationChecksum(UpperTankVolumeCalibration& calibration);
UpperTankVolumeResult estimateUpperTankVolume(float levelCm, const UpperTankVolumeCalibration& calibration);
bool clearUpperTankTransferReference(const UpperTankVolumeCalibration& existing,
                                      UpperTankVolumeCalibration& updatedOut);
bool setUpperTankTransferReference(const UpperTankVolumeCalibration& existing,
                                    float currentLevelCm, UpperTankVolumeCalibration& updatedOut);
UpperTankVolumeEditStatus addUpperTankPostTransferPoint(
    const UpperTankVolumeCalibration& existing, float currentLevelCm,
    float actualTransferredLiters, UpperTankVolumeCalibration& updatedOut);

UpperTankVolumeEditStatus addUpperTankVolumeCalibrationPoint(
    const UpperTankVolumeCalibration& existing, float levelCm, float liters,
    bool absoluteVolumeConfirmed, UpperTankVolumeCalibration& updatedOut);

// Uses a manually recorded pre-transfer level and the actual litres entered by
// the operator after transfer. No estimate is made if the starting point is
// outside the calibrated range or lacks a confirmed absolute anchor.
bool deriveUpperTankPostTransferPoint(
    const UpperTankVolumeCalibration& calibration, float previousLevelCm,
    float currentLevelCm, float actualTransferredLiters, CalibrationPoint& pointOut);

} // namespace HWC
#endif
