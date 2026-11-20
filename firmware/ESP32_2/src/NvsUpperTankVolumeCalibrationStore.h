#ifndef HWC_NVS_UPPER_TANK_VOLUME_CALIBRATION_STORE_H
#define HWC_NVS_UPPER_TANK_VOLUME_CALIBRATION_STORE_H
#include <Preferences.h>
#include "UpperTankVolumeCalibration.h"
namespace HWC {
class NvsUpperTankVolumeCalibrationStore {
public:
    NvsUpperTankVolumeCalibrationStore();
    bool begin();
    void end();
    bool hasRecord();
    bool load(UpperTankVolumeCalibration& calibration);
    bool save(const UpperTankVolumeCalibration& calibration);
    // Removes only an existing invalid record; valid calibration is never deleted.
    bool discardCorruptRecord();
private:
    Preferences preferences_;
    bool opened_;
};
}
#endif
