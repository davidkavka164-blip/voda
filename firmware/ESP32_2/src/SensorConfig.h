#ifndef HWC_SENSOR_CONFIG_H
#define HWC_SENSOR_CONFIG_H

#include <stdint.h>

namespace HWC {

// Tracks the persisted ELECTRICAL sensor configuration used to convert 4-20 mA
// into a water-level height. This is not the well level-to-volume calibration curve.
enum SensorConfigLoadState : uint8_t {
    SENSOR_CONFIG_NVS_UNAVAILABLE = 0,
    SENSOR_CONFIG_MISSING_PROVISIONAL = 1,
    SENSOR_CONFIG_INVALID_PRESENT = 2,
    SENSOR_CONFIG_VALID_STORED = 3
};

// A valid level reading requires a usable electrical conversion configuration.
// The separate WellCalibration volume curve is never consulted by pump/transfer
// protection and only affects the litres estimate.
inline bool sensorConfigUsableForLevelMeasurement(SensorConfigLoadState state) {
    // A missing record means the installed sensor's height conversion has not
    // been calibrated and verified. Provisional defaults are diagnostic only.
    // Safety use is allowed only after a stored record has passed validation.
    return state == SENSOR_CONFIG_VALID_STORED;
}

// Electrical sensor calibration only. This is deliberately separate from
// any tank geometry or well level-to-volume curve.
struct SensorConfig {
    uint32_t magic;
    uint16_t version;
    float shuntOhms;
    float currentMinMilliAmps;
    float currentMaxMilliAmps;
    float currentToleranceMilliAmps;
    float sensorRangeCm;
    float calibrationScale;
    float calibrationOffsetCm;
    uint32_t checksum;
};

static const uint32_t SENSOR_CONFIG_MAGIC = 0x48574353UL; // "HWCS"
static const uint16_t SENSOR_CONFIG_VERSION = 1;

SensorConfig defaultSensorConfig();
bool isSensorConfigValid(const SensorConfig& config);
// Builds a recovery candidate from known electrical defaults, a previously
// established scale, and a simultaneous independent physical level reference.
// Offset is derived rather than guessed: reference - rawLevel * scale.
bool buildSensorConfigRecoveryCandidate(float scale, float rawLevelCm,
                                        float referenceLevelCm, SensorConfig& out);
uint32_t sensorConfigChecksum(const SensorConfig& config);
bool sensorConfigChecksumMatches(const SensorConfig& config);
void updateSensorConfigChecksum(SensorConfig& config);

} // namespace HWC

#endif
