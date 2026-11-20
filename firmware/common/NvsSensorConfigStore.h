#ifndef HWC_NVS_SENSOR_CONFIG_STORE_H
#define HWC_NVS_SENSOR_CONFIG_STORE_H

#include <Arduino.h>
#include <Preferences.h>
#include "SensorConfig.h"

namespace HWC {

class NvsSensorConfigStore {
public:
    NvsSensorConfigStore();
    bool begin();
    void end();
    bool hasRecord(const char* key);
    // Returns true only when a stored record passed version, range and checksum checks.
    bool load(const char* key, SensorConfig& out);
    // Saves a complete, validated record. Does not modify NVS on invalid input.
    bool save(const char* key, SensorConfig config);
    // Explicit service recovery: may replace only a missing or invalid record.
    // A valid stored calibration is never overwritten through this path.
    bool recover(const char* key, SensorConfig config);
private:
    Preferences preferences_;
    bool opened_;
};

} // namespace HWC

#endif
