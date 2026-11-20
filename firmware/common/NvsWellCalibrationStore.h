#ifndef HWC_NVS_WELL_CALIBRATION_STORE_H
#define HWC_NVS_WELL_CALIBRATION_STORE_H

#include <Arduino.h>
#include <Preferences.h>
#include "WellGeometry.h"
#include "WellCalibrationEditor.h"

namespace HWC {
enum WellCalibrationSaveStatus {
    WELL_CAL_SAVE_OK,
    WELL_CAL_SAVE_STORE_NOT_OPEN,
    WELL_CAL_SAVE_EXISTING_DATA_CORRUPT,
    WELL_CAL_SAVE_MERGE_REJECTED,
    WELL_CAL_SAVE_WRITE_FAILED,
    WELL_CAL_SAVE_VERIFY_FAILED
};

class NvsWellCalibrationStore {
public:
    NvsWellCalibrationStore();
    bool begin();
    void end();
    bool load(WellCalibration& out);
    bool save(WellCalibration calibration);
    WellCalibrationSaveStatus mergeAndSave(
        const WellGeometryConfig& geometry,
        const CalibrationPoint* newPoints,
        size_t newCount,
        bool newPointsHaveConfirmedAbsoluteVolumes,
        WellCalibration& savedOut);
private:
    Preferences preferences_;
    bool opened_;
};
}
#endif
