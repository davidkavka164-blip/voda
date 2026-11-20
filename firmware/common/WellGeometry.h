#ifndef HWC_WELL_GEOMETRY_H
#define HWC_WELL_GEOMETRY_H

#include <stddef.h>
#include <stdint.h>
#include "WaterMath.h"

namespace HWC {

static const uint32_t WELL_GEOMETRY_MAGIC = 0x48574347UL; // "HWCG"
static const uint16_t WELL_GEOMETRY_VERSION = 1;
static const uint32_t WELL_CALIBRATION_MAGIC = 0x48574357UL; // "HWCW"
static const uint16_t WELL_CALIBRATION_VERSION = 1;

// Internal diameter and usable water-column height, in centimetres.
// These dimensions produce an estimate, not a measured calibration.
struct WellGeometryConfig {
    uint32_t magic;
    uint16_t version;
    float internalDiameterCm;
    float usableDepthCm;
    uint32_t checksum;
};

struct WellCalibration {
    uint32_t magic;
    uint16_t version;
    uint16_t pointCount;
    uint8_t absoluteAnchorKnown;
    uint8_t reserved[3];
    CalibrationPoint points[MAX_WELL_CALIBRATION_POINTS];
    uint32_t checksum;
};

enum WellVolumeStatus {
    WELL_VOLUME_GEOMETRY_ESTIMATE,
    WELL_VOLUME_CALIBRATED_EXACT,
    WELL_VOLUME_CALIBRATED_INTERPOLATED,
    WELL_VOLUME_INVALID_LEVEL,
    WELL_VOLUME_INVALID_GEOMETRY,
    WELL_VOLUME_INVALID_CALIBRATION,
    WELL_VOLUME_OUT_OF_RANGE
};

struct WellVolumeResult {
    WellVolumeStatus status;
    float liters;
    bool absoluteVolumeKnown;
};

WellGeometryConfig defaultWellGeometryConfig();
bool isWellGeometryConfigValid(const WellGeometryConfig& config);
uint32_t wellGeometryChecksum(const WellGeometryConfig& config);
bool wellGeometryChecksumMatches(const WellGeometryConfig& config);
void updateWellGeometryChecksum(WellGeometryConfig& config);
WellCalibration emptyWellCalibration();
bool isWellCalibrationValid(const WellCalibration& calibration);
uint32_t wellCalibrationChecksum(const WellCalibration& calibration);
bool wellCalibrationChecksumMatches(const WellCalibration& calibration);
void updateWellCalibrationChecksum(WellCalibration& calibration);
bool estimateGeometryVolumeLiters(float waterHeightCm, const WellGeometryConfig& config, float& litersOut);

// Measured points take precedence within their measured range. Outside that
// range the result is a clearly labelled geometry estimate, shifted to meet the
// nearest measured boundary. No measured points are changed by geometry edits.
WellVolumeResult estimateWellVolume(const WellGeometryConfig& geometry,
                                    float waterHeightCm,
                                    const CalibrationPoint* measuredPoints,
                                    size_t measuredCount,
                                    bool knownAbsoluteAnchor);
WellVolumeResult estimateWellVolume(const WellGeometryConfig& geometry,
                                    float waterHeightCm,
                                    const WellCalibration& calibration);

} // namespace HWC
#endif
