#include "WellGeometry.h"
#include <math.h>

namespace HWC {
namespace {
uint32_t crc32Byte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}
uint32_t crc32Bytes(uint32_t crc, const void* data, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < length; ++i) crc = crc32Byte(crc, bytes[i]);
    return crc;
}
}

WellGeometryConfig defaultWellGeometryConfig() {
    WellGeometryConfig c = {};
    c.magic = WELL_GEOMETRY_MAGIC;
    c.version = WELL_GEOMETRY_VERSION;
    c.internalDiameterCm = 80.0f;
    c.usableDepthCm = 800.0f;
    updateWellGeometryChecksum(c);
    return c;
}

bool isWellGeometryConfigValid(const WellGeometryConfig& c) {
    return c.magic == WELL_GEOMETRY_MAGIC && c.version == WELL_GEOMETRY_VERSION &&
           isfinite(c.internalDiameterCm) && c.internalDiameterCm >= 20.0f && c.internalDiameterCm <= 500.0f &&
           isfinite(c.usableDepthCm) && c.usableDepthCm >= 20.0f && c.usableDepthCm <= 2000.0f;
}

uint32_t wellGeometryChecksum(const WellGeometryConfig& c) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crc32Bytes(crc, &c.magic, sizeof(c.magic));
    crc = crc32Bytes(crc, &c.version, sizeof(c.version));
    crc = crc32Bytes(crc, &c.internalDiameterCm, sizeof(c.internalDiameterCm));
    crc = crc32Bytes(crc, &c.usableDepthCm, sizeof(c.usableDepthCm));
    return crc ^ 0xFFFFFFFFUL;
}

bool wellGeometryChecksumMatches(const WellGeometryConfig& c) {
    return c.checksum == wellGeometryChecksum(c);
}

void updateWellGeometryChecksum(WellGeometryConfig& c) { c.checksum = wellGeometryChecksum(c); }

WellCalibration emptyWellCalibration() {
    WellCalibration c = {};
    c.magic = WELL_CALIBRATION_MAGIC;
    c.version = WELL_CALIBRATION_VERSION;
    c.pointCount = 0;
    c.absoluteAnchorKnown = 0;
    updateWellCalibrationChecksum(c);
    return c;
}

bool isWellCalibrationValid(const WellCalibration& c) {
    if (c.magic != WELL_CALIBRATION_MAGIC || c.version != WELL_CALIBRATION_VERSION ||
        c.pointCount > MAX_WELL_CALIBRATION_POINTS || c.absoluteAnchorKnown > 1) return false;
    if (c.pointCount == 1) return false;
    if (c.pointCount >= 2 && !validateCalibrationCurve(c.points, c.pointCount)) return false;
    return true;
}

uint32_t wellCalibrationChecksum(const WellCalibration& c) {
    if (c.pointCount > MAX_WELL_CALIBRATION_POINTS) return 0;
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crc32Bytes(crc, &c.magic, sizeof(c.magic));
    crc = crc32Bytes(crc, &c.version, sizeof(c.version));
    crc = crc32Bytes(crc, &c.pointCount, sizeof(c.pointCount));
    crc = crc32Bytes(crc, &c.absoluteAnchorKnown, sizeof(c.absoluteAnchorKnown));
    crc = crc32Bytes(crc, c.points, sizeof(CalibrationPoint) * c.pointCount);
    return crc ^ 0xFFFFFFFFUL;
}

bool wellCalibrationChecksumMatches(const WellCalibration& c) {
    return c.checksum == wellCalibrationChecksum(c);
}

void updateWellCalibrationChecksum(WellCalibration& c) { c.checksum = wellCalibrationChecksum(c); }

bool estimateGeometryVolumeLiters(float waterHeightCm, const WellGeometryConfig& c, float& litersOut) {
    if (!isWellGeometryConfigValid(c) || !wellGeometryChecksumMatches(c)) return false;
    if (!isfinite(waterHeightCm) || waterHeightCm < 0.0f || waterHeightCm > c.usableDepthCm) return false;
    const float radius = c.internalDiameterCm / 2.0f;
    const float areaCm2 = 3.14159265358979323846f * radius * radius;
    litersOut = areaCm2 * waterHeightCm / 1000.0f;
    return isfinite(litersOut);
}

WellVolumeResult estimateWellVolume(const WellGeometryConfig& geometry, float level,
                                    const CalibrationPoint* points, size_t count,
                                    bool knownAbsoluteAnchor) {
    WellVolumeResult out = {WELL_VOLUME_INVALID_LEVEL, 0.0f, knownAbsoluteAnchor};
    if (!isfinite(level) || level < 0.0f) return out;
    if (!isWellGeometryConfigValid(geometry) || !wellGeometryChecksumMatches(geometry)) {
        out.status = WELL_VOLUME_INVALID_GEOMETRY;
        return out;
    }
    if (level > geometry.usableDepthCm) {
        out.status = WELL_VOLUME_OUT_OF_RANGE;
        return out;
    }
    if (count > 0 && !validateCalibrationCurve(points, count)) {
        out.status = WELL_VOLUME_INVALID_CALIBRATION;
        return out;
    }
    if (count > 0 && points[count - 1].levelCm > geometry.usableDepthCm) {
        out.status = WELL_VOLUME_INVALID_CALIBRATION;
        return out;
    }

    if (count > 0 && level >= points[0].levelCm && level <= points[count - 1].levelCm) {
        float calibratedLiters = 0.0f;
        const EstimateStatus s = estimateWellVolumeLiters(level, points, count, calibratedLiters);
        if (s == ESTIMATE_OK_EXACT || s == ESTIMATE_OK_INTERPOLATED) {
            // The basic cylindrical geometry is a hard minimum. Real cavities
            // and widenings may add water, so calibration may raise the result
            // but must never reduce it below the geometric baseline.
            float geometricMinimum = 0.0f;
            if (!estimateGeometryVolumeLiters(level, geometry, geometricMinimum)) {
                out.status = WELL_VOLUME_OUT_OF_RANGE;
                return out;
            }
            out.status = (s == ESTIMATE_OK_EXACT) ? WELL_VOLUME_CALIBRATED_EXACT : WELL_VOLUME_CALIBRATED_INTERPOLATED;
            out.liters = calibratedLiters > geometricMinimum ? calibratedLiters : geometricMinimum;
            return out;
        }
        out.status = WELL_VOLUME_INVALID_CALIBRATION;
        return out;
    }

    float geometryLiters = 0.0f;
    if (!estimateGeometryVolumeLiters(level, geometry, geometryLiters)) {
        out.status = WELL_VOLUME_OUT_OF_RANGE;
        return out;
    }

    // Keep the estimate continuous at the nearest measured boundary, while
    // clearly retaining its estimated status outside the measured range.
    if (count >= 2) {
        const size_t anchor = (level < points[0].levelCm) ? 0 : count - 1;
        float geometricAnchor = 0.0f;
        if (estimateGeometryVolumeLiters(points[anchor].levelCm, geometry, geometricAnchor)) {
            // Carry only positive cavity/widening volume beyond the geometric
            // minimum. A low or imperfect historical point cannot subtract
            // water from the basic geometry model.
            const float additionalCavityLiters = points[anchor].volumeLiters > geometricAnchor
                ? points[anchor].volumeLiters - geometricAnchor : 0.0f;
            geometryLiters += additionalCavityLiters;
        }
    }
    out.status = WELL_VOLUME_GEOMETRY_ESTIMATE;
    out.liters = geometryLiters;
    return out;
}

WellVolumeResult estimateWellVolume(const WellGeometryConfig& geometry, float level,
                                    const WellCalibration& calibration) {
    if (!isWellCalibrationValid(calibration) || !wellCalibrationChecksumMatches(calibration)) {
        WellVolumeResult out = {WELL_VOLUME_INVALID_CALIBRATION, 0.0f, false};
        return out;
    }
    return estimateWellVolume(geometry, level,
                              calibration.pointCount >= 2 ? calibration.points : nullptr,
                              calibration.pointCount,
                              calibration.absoluteAnchorKnown != 0);
}

} // namespace HWC
