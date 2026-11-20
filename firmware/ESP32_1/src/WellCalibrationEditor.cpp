#include "WellCalibrationEditor.h"

namespace HWC {

WellCalibrationEditStatus mergeWellCalibration(
    const WellGeometryConfig& geometry,
    const WellCalibration& existing,
    const CalibrationPoint* newPoints,
    size_t newCount,
    bool newPointsHaveConfirmedAbsoluteVolumes,
    WellCalibration& mergedOut) {

    if (!isWellGeometryConfigValid(geometry) || !wellGeometryChecksumMatches(geometry))
        return WELL_CAL_EDIT_INVALID_GEOMETRY;
    if (!isWellCalibrationValid(existing) || !wellCalibrationChecksumMatches(existing))
        return WELL_CAL_EDIT_INVALID_EXISTING;
    if (!validateCalibrationCurve(newPoints, newCount))
        return WELL_CAL_EDIT_INVALID_NEW_POINTS;
    if (newPoints[0].levelCm > geometry.usableDepthCm ||
        newPoints[newCount - 1].levelCm > geometry.usableDepthCm)
        return WELL_CAL_EDIT_POINT_OUTSIDE_DEPTH;

    // A known absolute curve cannot be partly replaced with points whose volume
    // reference has not been confirmed. That would silently mix two references.
    if (existing.absoluteAnchorKnown && !newPointsHaveConfirmedAbsoluteVolumes)
        return WELL_CAL_EDIT_ANCHOR_CONFLICT;

    CalibrationPoint candidatePoints[MAX_WELL_CALIBRATION_POINTS];
    size_t count = 0;
    const float minLevel = newPoints[0].levelCm;
    const float maxLevel = newPoints[newCount - 1].levelCm;

    for (size_t i = 0; i < existing.pointCount; ++i) {
        if (existing.points[i].levelCm < minLevel) {
            if (count >= MAX_WELL_CALIBRATION_POINTS) return WELL_CAL_EDIT_TOO_MANY_POINTS;
            candidatePoints[count++] = existing.points[i];
        }
    }
    for (size_t i = 0; i < newCount; ++i) {
        if (count >= MAX_WELL_CALIBRATION_POINTS) return WELL_CAL_EDIT_TOO_MANY_POINTS;
        candidatePoints[count++] = newPoints[i];
    }
    for (size_t i = 0; i < existing.pointCount; ++i) {
        if (existing.points[i].levelCm > maxLevel) {
            if (count >= MAX_WELL_CALIBRATION_POINTS) return WELL_CAL_EDIT_TOO_MANY_POINTS;
            candidatePoints[count++] = existing.points[i];
        }
    }

    if (count < 2 || !validateCalibrationCurve(candidatePoints, count))
        return WELL_CAL_EDIT_VOLUME_CONFLICT;

    WellCalibration candidate = emptyWellCalibration();
    candidate.pointCount = static_cast<uint16_t>(count);
    for (size_t i = 0; i < count; ++i) candidate.points[i] = candidatePoints[i];

    // An absolute anchor is trusted only if it was already known and preserved,
    // or this edit defines the whole curve from confirmed absolute measurements.
    const bool oldPointsRemain =
        (existing.pointCount > 0 && existing.points[0].levelCm < minLevel) ||
        (existing.pointCount > 0 && existing.points[existing.pointCount - 1].levelCm > maxLevel);
    candidate.absoluteAnchorKnown = (existing.absoluteAnchorKnown != 0) ? 1 :
        ((!oldPointsRemain && newPointsHaveConfirmedAbsoluteVolumes) ? 1 : 0);
    updateWellCalibrationChecksum(candidate);
    if (!isWellCalibrationValid(candidate) || !wellCalibrationChecksumMatches(candidate))
        return WELL_CAL_EDIT_INVALID_NEW_POINTS;

    mergedOut = candidate;
    return WELL_CAL_EDIT_OK;
}

} // namespace HWC
