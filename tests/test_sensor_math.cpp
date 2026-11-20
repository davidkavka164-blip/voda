#include "../firmware/common/SensorMath.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

using namespace HWC;

static SensorMathResult convert(float mV, float scale = 1.0f, float offset = 0.0f) {
    return levelFromShuntMilliVolts(mV, 150.0f, 4.0f, 20.0f, 0.5f, 1000.0f, scale, offset, 800.0f);
}

int main() {
    SensorMathResult r = convert(600.0f);
    assert(r.status == SENSOR_MATH_OK);
    assert(fabsf(r.currentMilliAmps - 4.0f) < 0.001f);
    assert(fabsf(r.rawLevelCm) < 0.01f);

    r = convert(1800.0f);
    assert(r.status == SENSOR_MATH_OK);
    assert(fabsf(r.currentMilliAmps - 12.0f) < 0.001f);
    assert(fabsf(r.rawLevelCm - 500.0f) < 0.1f);

    r = convert(3000.0f);
    // 20 mA corresponds to 1000 cm, beyond the current 800 cm configured well depth.
    assert(r.status == SENSOR_MATH_LEVEL_OUT_OF_RANGE);

    r = convert(450.0f);
    assert(r.status == SENSOR_MATH_CURRENT_OUT_OF_RANGE); // 3 mA is a wire/sensor fault.

    r = convert(1200.0f, 1.0f, -100.0f);
    assert(r.status == SENSOR_MATH_OK);
    assert(fabsf(r.calibratedLevelCm - 150.0f) < 0.1f);

    puts("PASS: SensorMath native tests");
    return 0;
}
