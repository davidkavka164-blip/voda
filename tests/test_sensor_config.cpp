#include "SensorConfig.h"
#include <assert.h>
#include <stdio.h>

int main() {
    HWC::SensorConfig config = HWC::defaultSensorConfig();
    assert(HWC::isSensorConfigValid(config));
    assert(HWC::sensorConfigChecksumMatches(config));
    assert(config.shuntOhms == 150.0f);
    assert(config.sensorRangeCm == 1000.0f);

    // Default values are diagnostic/commissioning values only. The sensor
    // must have a stored, validated height conversion before safety use.
    assert(!HWC::sensorConfigUsableForLevelMeasurement(HWC::SENSOR_CONFIG_MISSING_PROVISIONAL));
    assert(HWC::sensorConfigUsableForLevelMeasurement(HWC::SENSOR_CONFIG_VALID_STORED));
    assert(!HWC::sensorConfigUsableForLevelMeasurement(HWC::SENSOR_CONFIG_INVALID_PRESENT));
    assert(!HWC::sensorConfigUsableForLevelMeasurement(HWC::SENSOR_CONFIG_NVS_UNAVAILABLE));

    HWC::SensorConfig edited = config;
    edited.calibrationScale = 1.02f;
    edited.calibrationOffsetCm = -3.5f;
    HWC::updateSensorConfigChecksum(edited);
    assert(HWC::isSensorConfigValid(edited));
    assert(HWC::sensorConfigChecksumMatches(edited));

    edited.calibrationOffsetCm = -9999.0f;
    assert(!HWC::isSensorConfigValid(edited));
    edited = config;
    edited.shuntOhms = 0.0f;
    assert(!HWC::isSensorConfigValid(edited));
    edited = config;
    edited.checksum ^= 0x01U;
    assert(!HWC::sensorConfigChecksumMatches(edited));

    puts("PASS: SensorConfig validation and checksum tests");
    HWC::SensorConfig recovery = {};
    assert(HWC::buildSensorConfigRecoveryCandidate(1.0f, 120.0f, 125.0f, recovery));
    assert(recovery.calibrationOffsetCm == 5.0f);
    assert(HWC::isSensorConfigValid(recovery) && HWC::sensorConfigChecksumMatches(recovery));
    assert(!HWC::buildSensorConfigRecoveryCandidate(1.6f, 120.0f, 125.0f, recovery));
    return 0;
}
