#pragma once

#include <stdint.h>

namespace HWC {

enum WaterTemperatureStatus {
    WATER_TEMPERATURE_NOT_INITIALIZED = 0,
    WATER_TEMPERATURE_VALID,
    WATER_TEMPERATURE_SENSOR_MISSING,
    WATER_TEMPERATURE_READ_ERROR,
    WATER_TEMPERATURE_OUT_OF_RANGE
};

struct WaterTemperatureReading {
    float temperatureC;
    uint32_t sampledAtMs;
    WaterTemperatureStatus status;
};

WaterTemperatureReading emptyWaterTemperatureReading();
bool isWaterTemperatureValueValid(float temperatureC);
void updateWaterTemperatureReading(WaterTemperatureReading& reading,
                                   bool sensorPresent,
                                   bool readSucceeded,
                                   float temperatureC,
                                   uint32_t nowMs);
bool isWaterTemperatureFresh(const WaterTemperatureReading& reading,
                             uint32_t nowMs,
                             uint32_t maximumAgeMs);

} // namespace HWC
