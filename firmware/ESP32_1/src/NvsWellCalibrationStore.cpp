#include "NvsWellCalibrationStore.h"

namespace {
bool sameCalibration(const HWC::WellCalibration& a, const HWC::WellCalibration& b) {
    if (a.magic != b.magic || a.version != b.version ||
        a.pointCount != b.pointCount ||
        a.absoluteAnchorKnown != b.absoluteAnchorKnown ||
        a.reserved[0] != b.reserved[0] ||
        a.reserved[1] != b.reserved[1] ||
        a.reserved[2] != b.reserved[2] ||
        a.checksum != b.checksum) return false;
    for (size_t i = 0; i < a.pointCount; ++i) {
        if (a.points[i].levelCm != b.points[i].levelCm ||
            a.points[i].volumeLiters != b.points[i].volumeLiters) return false;
    }
    return true;
}
}

namespace HWC {
NvsWellCalibrationStore::NvsWellCalibrationStore() : opened_(false) {}

bool NvsWellCalibrationStore::begin() {
    if (opened_) return true;
    opened_ = preferences_.begin("hwc_well", false);
    return opened_;
}

void NvsWellCalibrationStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsWellCalibrationStore::load(WellCalibration& out) {
    if (!opened_ || !preferences_.isKey("calibration")) return false;
    if (preferences_.getBytesLength("calibration") != sizeof(WellCalibration)) return false;
    WellCalibration candidate = {};
    if (preferences_.getBytes("calibration", &candidate, sizeof(candidate)) != sizeof(candidate)) return false;
    if (!isWellCalibrationValid(candidate) || !wellCalibrationChecksumMatches(candidate)) return false;
    out = candidate;
    return true;
}

bool NvsWellCalibrationStore::save(WellCalibration calibration) {
    if (!opened_ || !isWellCalibrationValid(calibration)) return false;
    if (preferences_.isKey("calibration")) {
        WellCalibration existing = emptyWellCalibration();
        if (!load(existing)) return false;
    }
    updateWellCalibrationChecksum(calibration);
    if (preferences_.putBytes("calibration", &calibration, sizeof(calibration)) != sizeof(calibration)) return false;

    // Keep direct save() as strict as mergeAndSave(): verify persisted bytes.
    WellCalibration verify = emptyWellCalibration();
    return load(verify) && sameCalibration(verify, calibration);
}

WellCalibrationSaveStatus NvsWellCalibrationStore::mergeAndSave(
    const WellGeometryConfig& geometry,
    const CalibrationPoint* newPoints,
    size_t newCount,
    bool newPointsHaveConfirmedAbsoluteVolumes,
    WellCalibration& savedOut) {
    if (!opened_) return WELL_CAL_SAVE_STORE_NOT_OPEN;

    WellCalibration existing = emptyWellCalibration();
    if (preferences_.isKey("calibration")) {
        if (!load(existing)) return WELL_CAL_SAVE_EXISTING_DATA_CORRUPT;
    }

    WellCalibration candidate = emptyWellCalibration();
    const WellCalibrationEditStatus editStatus = mergeWellCalibration(
        geometry, existing, newPoints, newCount,
        newPointsHaveConfirmedAbsoluteVolumes, candidate);
    if (editStatus != WELL_CAL_EDIT_OK) return WELL_CAL_SAVE_MERGE_REJECTED;

    if (!save(candidate)) return WELL_CAL_SAVE_WRITE_FAILED;

    // Verify the persisted record before reporting success to the caller.
    WellCalibration verified = emptyWellCalibration();
    if (!load(verified) || !sameCalibration(verified, candidate)) {
        return WELL_CAL_SAVE_VERIFY_FAILED;
    }
    savedOut = verified;
    return WELL_CAL_SAVE_OK;
}
}
