#include "WaterTemperature.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
using namespace HWC;
int main() {
    WaterTemperatureReading r = emptyWaterTemperatureReading();
    assert(r.status == WATER_TEMPERATURE_NOT_INITIALIZED);
    assert(!isWaterTemperatureFresh(r, 0U, 15000U));
    assert(isWaterTemperatureValueValid(-55.0f));
    assert(isWaterTemperatureValueValid(125.0f));
    assert(!isWaterTemperatureValueValid(-127.0f));
    assert(!isWaterTemperatureValueValid(-85.0f));
    assert(!isWaterTemperatureValueValid(125.1f));
    assert(!isWaterTemperatureValueValid(NAN));
    updateWaterTemperatureReading(r, false, false, 0.0f, 100U);
    assert(r.status == WATER_TEMPERATURE_SENSOR_MISSING);
    updateWaterTemperatureReading(r, true, false, 0.0f, 200U);
    assert(r.status == WATER_TEMPERATURE_READ_ERROR);
    updateWaterTemperatureReading(r, true, true, 126.0f, 300U);
    assert(r.status == WATER_TEMPERATURE_OUT_OF_RANGE);
    updateWaterTemperatureReading(r, true, true, 18.75f, 1000U);
    assert(r.status == WATER_TEMPERATURE_VALID && r.temperatureC == 18.75f);
    assert(isWaterTemperatureFresh(r, 16000U, 15000U));
    assert(!isWaterTemperatureFresh(r, 16001U, 15000U));
    const uint32_t lastGoodSample = r.sampledAtMs;
    updateWaterTemperatureReading(r, true, false, 0.0f, 17000U);
    assert(!isWaterTemperatureFresh(r, 17000U, 15000U));
    assert(r.sampledAtMs == lastGoodSample); // failed reads must not refresh sample age

    updateWaterTemperatureReading(r, true, true, 7.5f, 0xFFFFFFF0U);
    assert(isWaterTemperatureFresh(r, 0x00000020U, 64U)); // millis() rollover
    assert(!isWaterTemperatureFresh(r, 0x00000040U, 64U));
    puts("PASS: water temperature validation, fault states and freshness");
}
