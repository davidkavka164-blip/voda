#ifndef HWC_WELL_CALIBRATION_EDITOR_H
#define HWC_WELL_CALIBRATION_EDITOR_H

#include "WellGeometry.h"

namespace HWC {

enum WellCalibrationEditStatus {
    WELL_CAL_EDIT_OK,
    WELL_CAL_EDIT_INVALID_EXISTING,
    WELL_CAL_EDIT_INVALID_GEOMETRY,
    WELL_CAL_EDIT_INVALID_NEW_POINTS,
    WELL_CAL_EDIT_POINT_OUTSIDE_DEPTH,
    WELL_CAL_EDIT_TOO_MANY_POINTS,
    WELL_CAL_EDIT_VOLUME_CONFLICT,
    WELL_CAL_EDIT_ANCHOR_CONFLICT
};

// Replaces the measured interval [newPoints[0].levelCm, newPoints[last].levelCm]
// while preserving all old points strictly outside that interval.
// Input arrays/records are never modified. The output is committed only on success.
WellCalibrationEditStatus mergeWellCalibration(
    const WellGeometryConfig& geometry,
    const WellCalibration& existing,
    const CalibrationPoint* newPoints,
    size_t newCount,
    bool newPointsHaveConfirmedAbsoluteVolumes,
    WellCalibration& mergedOut);

} // namespace HWC
#endif
