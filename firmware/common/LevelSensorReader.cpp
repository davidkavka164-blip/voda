#include "LevelSensorReader.h"

namespace HWC {

LevelSensorReader::LevelSensorReader(uint8_t pin, const SensorConfig& config)
    : pin_(pin), config_(config) {}

void LevelSensorReader::begin() {
    pinMode(pin_, INPUT);
    analogSetPinAttenuation(pin_, ADC_11db);
}

void LevelSensorReader::setConfig(const SensorConfig& config) {
    if (isSensorConfigValid(config) && sensorConfigChecksumMatches(config)) config_ = config;
}

LevelSensorReading LevelSensorReader::read() {
    LevelSensorReading reading = {};
    reading.adcReadOk = false;
    reading.millivolts = 0;
    reading.result.status = SENSOR_MATH_INVALID_INPUT;
    reading.result.currentMilliAmps = 0.0f;
    reading.result.rawLevelCm = 0.0f;
    reading.result.calibratedLevelCm = 0.0f;

    if (!isSensorConfigValid(config_) || !sensorConfigChecksumMatches(config_)) return reading;

    // Average eight calibrated ADC voltage readings to reduce short noise spikes.
    uint32_t totalMilliVolts = 0;
    for (uint8_t i = 0; i < 8; ++i) {
        totalMilliVolts += analogReadMilliVolts(pin_);
        delayMicroseconds(250);
    }
    reading.millivolts = static_cast<uint16_t>((totalMilliVolts + 4U) / 8U);
    reading.adcReadOk = true;

    // Physical tank/well maximums are applied by the domain layer, once each
    // sensor's installation and geometry are confirmed. This limit only rejects
    // grossly impossible calibrated values at this electrical-conversion layer.
    const float broadSanityLimitCm = 3000.0f;
    reading.result = levelFromShuntMilliVolts(
        static_cast<float>(reading.millivolts),
        config_.shuntOhms,
        config_.currentMinMilliAmps,
        config_.currentMaxMilliAmps,
        config_.currentToleranceMilliAmps,
        config_.sensorRangeCm,
        config_.calibrationScale,
        config_.calibrationOffsetCm,
        broadSanityLimitCm);
    return reading;
}

} // namespace HWC
