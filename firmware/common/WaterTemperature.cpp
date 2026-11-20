#include "WaterTemperature.h"
#include <math.h>

namespace HWC {

WaterTemperatureReading emptyWaterTemperatureReading() {
    WaterTemperatureReading reading = {};
    reading.temperatureC = 0.0f;
    reading.sampledAtMs = 0U;
    reading.status = WATER_TEMPERATURE_NOT_INITIALIZED;
    return reading;
}

bool isWaterTemperatureValueValid(float temperatureC) {
    return isfinite(temperatureC) && temperatureC >= -55.0f && temperatureC <= 125.0f;
}

void updateWaterTemperatureReading(WaterTemperatureReading& reading,
                                   bool sensorPresent,
                                   bool readSucceeded,
                                   float temperatureC,
                                   uint32_t nowMs) {
    if (!sensorPresent) {
        reading.status = WATER_TEMPERATURE_SENSOR_MISSING;
        return;
    }
    if (!readSucceeded) {
        reading.status = WATER_TEMPERATURE_READ_ERROR;
        return;
    }
    if (!isWaterTemperatureValueValid(temperatureC)) {
        reading.status = WATER_TEMPERATURE_OUT_OF_RANGE;
        return;
    }
    reading.temperatureC = temperatureC;
    reading.sampledAtMs = nowMs;
    reading.status = WATER_TEMPERATURE_VALID;
}

bool isWaterTemperatureFresh(const WaterTemperatureReading& reading,
                             uint32_t nowMs,
                             uint32_t maximumAgeMs) {
    return reading.status == WATER_TEMPERATURE_VALID &&
           static_cast<uint32_t>(nowMs - reading.sampledAtMs) <= maximumAgeMs;
}

} // namespace HWC
