#include "UpperTankVolumeCalibration.h"
#include <math.h>

namespace HWC {
namespace {
uint32_t crcByte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}
uint32_t crcBytes(uint32_t crc, const void* data, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < length; ++i) crc = crcByte(crc, bytes[i]);
    return crc;
}
bool validPoints(const CalibrationPoint* points, size_t count) {
    if (count > MAX_UPPER_VOLUME_CAL_POINTS) return false;
    for (size_t i = 0; i < count; ++i) {
        if (!isfinite(points[i].levelCm) || !isfinite(points[i].volumeLiters) ||
            points[i].levelCm < 0.0f || points[i].volumeLiters < 0.0f) return false;
        if (i && (points[i].levelCm <= points[i - 1].levelCm ||
                  points[i].volumeLiters <= points[i - 1].volumeLiters)) return false;
    }
    return true;
}
}

UpperTankVolumeCalibration emptyUpperTankVolumeCalibration() {
    UpperTankVolumeCalibration c = {};
    c.magic = UPPER_VOLUME_CAL_MAGIC;
    c.version = UPPER_VOLUME_CAL_VERSION;
    updateUpperTankVolumeCalibrationChecksum(c);
    return c;
}

bool isUpperTankVolumeCalibrationValid(const UpperTankVolumeCalibration& c) {
    return c.magic == UPPER_VOLUME_CAL_MAGIC && c.version == UPPER_VOLUME_CAL_VERSION &&
           c.pointCount <= MAX_UPPER_VOLUME_CAL_POINTS && c.absoluteAnchorKnown <= 1 && c.transferReferenceValid <= 1 &&
           ((c.transferReferenceValid && c.absoluteAnchorKnown == 1 &&
             isfinite(c.transferReferenceLevelCm) && c.transferReferenceLevelCm >= 0.0f &&
             isfinite(c.transferReferenceVolumeLiters) && c.transferReferenceVolumeLiters >= 0.0f) ||
            (!c.transferReferenceValid && c.transferReferenceLevelCm == 0.0f && c.transferReferenceVolumeLiters == 0.0f)) &&
           validPoints(c.points, c.pointCount) &&
           (c.pointCount == 0 || c.absoluteAnchorKnown == 1);
}

uint32_t upperTankVolumeCalibrationChecksum(const UpperTankVolumeCalibration& c) {
    if (c.pointCount > MAX_UPPER_VOLUME_CAL_POINTS) return 0;
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcBytes(crc, &c.magic, sizeof(c.magic));
    crc = crcBytes(crc, &c.version, sizeof(c.version));
    crc = crcBytes(crc, &c.pointCount, sizeof(c.pointCount));
    crc = crcBytes(crc, &c.absoluteAnchorKnown, sizeof(c.absoluteAnchorKnown));
    crc = crcBytes(crc, &c.transferReferenceValid, sizeof(c.transferReferenceValid));
    crc = crcBytes(crc, &c.transferReferenceLevelCm, sizeof(c.transferReferenceLevelCm));
    crc = crcBytes(crc, &c.transferReferenceVolumeLiters, sizeof(c.transferReferenceVolumeLiters));
    crc = crcBytes(crc, c.points, sizeof(CalibrationPoint) * c.pointCount);
    return crc ^ 0xFFFFFFFFUL;
}

bool upperTankVolumeCalibrationChecksumMatches(const UpperTankVolumeCalibration& c) {
    return c.checksum == upperTankVolumeCalibrationChecksum(c);
}
void updateUpperTankVolumeCalibrationChecksum(UpperTankVolumeCalibration& c) {
    c.checksum = upperTankVolumeCalibrationChecksum(c);
}

UpperTankVolumeResult estimateUpperTankVolume(float level, const UpperTankVolumeCalibration& c) {
    UpperTankVolumeResult out = {UPPER_VOLUME_INVALID_CALIBRATION, 0.0f, false};
    if (!isUpperTankVolumeCalibrationValid(c) || !upperTankVolumeCalibrationChecksumMatches(c)) return out;
    if (!isfinite(level) || level < 0.0f) { out.status = UPPER_VOLUME_INVALID_LEVEL; return out; }
    if (!c.pointCount) { out.status = UPPER_VOLUME_NOT_CONFIGURED; return out; }
    if (!c.absoluteAnchorKnown) { out.status = UPPER_VOLUME_ABSOLUTE_ANCHOR_UNKNOWN; return out; }
    out.absoluteVolumeKnown = true;
    if (c.pointCount == 1) {
        if (fabsf(level - c.points[0].levelCm) <= 0.001f) {
            out.status = UPPER_VOLUME_EXACT;
            out.liters = c.points[0].volumeLiters;
        } else out.status = UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE;
        return out;
    }
    if (level < c.points[0].levelCm || level > c.points[c.pointCount - 1].levelCm) {
        out.status = UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE;
        return out;
    }
    for (size_t i = 0; i < c.pointCount; ++i) {
        if (fabsf(level - c.points[i].levelCm) <= 0.001f) {
            out.status = UPPER_VOLUME_EXACT;
            out.liters = c.points[i].volumeLiters;
            return out;
        }
        if (i + 1 < c.pointCount && level > c.points[i].levelCm && level < c.points[i + 1].levelCm) {
            const float fraction = (level - c.points[i].levelCm) /
                (c.points[i + 1].levelCm - c.points[i].levelCm);
            out.liters = c.points[i].volumeLiters + fraction *
                (c.points[i + 1].volumeLiters - c.points[i].volumeLiters);
            out.status = UPPER_VOLUME_INTERPOLATED;
            return out;
        }
    }
    out.status = UPPER_VOLUME_OUTSIDE_CALIBRATED_RANGE;
    return out;
}

UpperTankVolumeEditStatus addUpperTankVolumeCalibrationPoint(
    const UpperTankVolumeCalibration& existing, float level, float liters,
    bool confirmed, UpperTankVolumeCalibration& updatedOut) {
    if (!isUpperTankVolumeCalibrationValid(existing) ||
        !upperTankVolumeCalibrationChecksumMatches(existing)) return UPPER_VOLUME_EDIT_INVALID_EXISTING;
    // Do not let a manual table edit silently discard a pending transfer reference.
    // The post-transfer path explicitly clears the reference in its local copy first.
    if (existing.transferReferenceValid) return UPPER_VOLUME_EDIT_CONFLICT;
    if (!isfinite(level) || !isfinite(liters) || level < 0.0f || liters < 0.0f)
        return UPPER_VOLUME_EDIT_INVALID_POINT;
    if (!confirmed) return UPPER_VOLUME_EDIT_NEEDS_ABSOLUTE_CONFIRMATION;

    CalibrationPoint candidate[MAX_UPPER_VOLUME_CAL_POINTS];
    size_t count = 0;
    bool inserted = false;
    for (size_t i = 0; i < existing.pointCount; ++i) {
        if (fabsf(existing.points[i].levelCm - level) <= 0.001f) {
            if (inserted) return UPPER_VOLUME_EDIT_CONFLICT;
            candidate[count++] = {level, liters};
            inserted = true;
        } else {
            if (!inserted && level < existing.points[i].levelCm) {
                if (count >= MAX_UPPER_VOLUME_CAL_POINTS) return UPPER_VOLUME_EDIT_TOO_MANY_POINTS;
                candidate[count++] = {level, liters};
                inserted = true;
            }
            if (count >= MAX_UPPER_VOLUME_CAL_POINTS) return UPPER_VOLUME_EDIT_TOO_MANY_POINTS;
            candidate[count++] = existing.points[i];
        }
    }
    if (!inserted) {
        if (count >= MAX_UPPER_VOLUME_CAL_POINTS) return UPPER_VOLUME_EDIT_TOO_MANY_POINTS;
        candidate[count++] = {level, liters};
    }
    if (!validPoints(candidate, count)) return UPPER_VOLUME_EDIT_CONFLICT;

    UpperTankVolumeCalibration updated = emptyUpperTankVolumeCalibration();
    updated.pointCount = static_cast<uint16_t>(count);
    updated.absoluteAnchorKnown = 1;
    updated.transferReferenceValid = 0;
    updated.transferReferenceLevelCm = 0.0f;
    updated.transferReferenceVolumeLiters = 0.0f;
    for (size_t i = 0; i < count; ++i) updated.points[i] = candidate[i];
    updateUpperTankVolumeCalibrationChecksum(updated);
    updatedOut = updated;
    return UPPER_VOLUME_EDIT_OK;
}

bool clearUpperTankTransferReference(const UpperTankVolumeCalibration& existing,
                                      UpperTankVolumeCalibration& updatedOut) {
    if (!isUpperTankVolumeCalibrationValid(existing) ||
        !upperTankVolumeCalibrationChecksumMatches(existing)) return false;
    UpperTankVolumeCalibration updated = existing;
    updated.transferReferenceValid = 0;
    updated.transferReferenceLevelCm = 0.0f;
    updated.transferReferenceVolumeLiters = 0.0f;
    updateUpperTankVolumeCalibrationChecksum(updated);
    updatedOut = updated;
    return true;
}

bool setUpperTankTransferReference(const UpperTankVolumeCalibration& existing,
                                    float currentLevel, UpperTankVolumeCalibration& updatedOut) {
    if (!isUpperTankVolumeCalibrationValid(existing) ||
        !upperTankVolumeCalibrationChecksumMatches(existing)) return false;
    const UpperTankVolumeResult volume = estimateUpperTankVolume(currentLevel, existing);
    if (!volume.absoluteVolumeKnown ||
        (volume.status != UPPER_VOLUME_EXACT && volume.status != UPPER_VOLUME_INTERPOLATED)) return false;
    UpperTankVolumeCalibration updated = existing;
    updated.transferReferenceValid = 1;
    updated.transferReferenceLevelCm = currentLevel;
    updated.transferReferenceVolumeLiters = volume.liters;
    updateUpperTankVolumeCalibrationChecksum(updated);
    updatedOut = updated;
    return true;
}

UpperTankVolumeEditStatus addUpperTankPostTransferPoint(
    const UpperTankVolumeCalibration& existing, float currentLevel,
    float transferredLiters, UpperTankVolumeCalibration& updatedOut) {
    if (!isUpperTankVolumeCalibrationValid(existing) ||
        !upperTankVolumeCalibrationChecksumMatches(existing)) return UPPER_VOLUME_EDIT_INVALID_EXISTING;
    if (!existing.transferReferenceValid) return UPPER_VOLUME_EDIT_INVALID_POINT;
    CalibrationPoint point = {};
    const float newVolume = existing.transferReferenceVolumeLiters - transferredLiters;
    if (!isfinite(currentLevel) || !isfinite(transferredLiters) || transferredLiters <= 0.0f ||
        currentLevel < 0.0f || currentLevel >= existing.transferReferenceLevelCm ||
        !isfinite(newVolume) || newVolume < 0.0f) return UPPER_VOLUME_EDIT_INVALID_POINT;
    point.levelCm = currentLevel;
    point.volumeLiters = newVolume;
    UpperTankVolumeCalibration base = existing;
    base.transferReferenceValid = 0;
    base.transferReferenceLevelCm = 0.0f;
    base.transferReferenceVolumeLiters = 0.0f;
    updateUpperTankVolumeCalibrationChecksum(base);
    return addUpperTankVolumeCalibrationPoint(base, point.levelCm, point.volumeLiters, true, updatedOut);
}

bool deriveUpperTankPostTransferPoint(
    const UpperTankVolumeCalibration& calibration, float previousLevel,
    float currentLevel, float transferredLiters, CalibrationPoint& pointOut) {
    if (!isfinite(currentLevel) || !isfinite(transferredLiters) || transferredLiters <= 0.0f ||
        currentLevel < 0.0f || currentLevel >= previousLevel) return false;
    const UpperTankVolumeResult previous = estimateUpperTankVolume(previousLevel, calibration);
    if (!previous.absoluteVolumeKnown ||
        (previous.status != UPPER_VOLUME_EXACT && previous.status != UPPER_VOLUME_INTERPOLATED)) return false;
    const float newVolume = previous.liters - transferredLiters;
    if (!isfinite(newVolume) || newVolume < 0.0f) return false;
    pointOut.levelCm = currentLevel;
    pointOut.volumeLiters = newVolume;
    return true;
}

} // namespace HWC
