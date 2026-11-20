#include "SensorMath.h"
#include <math.h>

namespace HWC {

SensorMathResult levelFromShuntMilliVolts(float shuntMilliVolts,
                                          float shuntOhms,
                                          float currentMinMilliAmps,
                                          float currentMaxMilliAmps,
                                          float currentToleranceMilliAmps,
                                          float sensorRangeCm,
                                          float calibrationScale,
                                          float calibrationOffsetCm,
                                          float maximumLevelCm) {
    SensorMathResult out;
    out.status = SENSOR_MATH_INVALID_INPUT;
    out.currentMilliAmps = 0.0f;
    out.rawLevelCm = 0.0f;
    out.calibratedLevelCm = 0.0f;

    if (!isfinite(shuntMilliVolts) || !isfinite(shuntOhms) ||
        !isfinite(currentMinMilliAmps) || !isfinite(currentMaxMilliAmps) ||
        !isfinite(currentToleranceMilliAmps) || !isfinite(sensorRangeCm) ||
        !isfinite(calibrationScale) || !isfinite(calibrationOffsetCm) ||
        !isfinite(maximumLevelCm) || shuntMilliVolts < 0.0f || shuntOhms <= 0.0f ||
        currentMaxMilliAmps <= currentMinMilliAmps || currentToleranceMilliAmps < 0.0f ||
        sensorRangeCm <= 0.0f || calibrationScale <= 0.0f || maximumLevelCm <= 0.0f) return out;

    out.currentMilliAmps = shuntMilliVolts / shuntOhms;
    if (out.currentMilliAmps < currentMinMilliAmps - currentToleranceMilliAmps ||
        out.currentMilliAmps > currentMaxMilliAmps + currentToleranceMilliAmps) {
        out.status = SENSOR_MATH_CURRENT_OUT_OF_RANGE;
        return out;
    }

    out.rawLevelCm = ((out.currentMilliAmps - currentMinMilliAmps) /
                      (currentMaxMilliAmps - currentMinMilliAmps)) * sensorRangeCm;
    out.calibratedLevelCm = out.rawLevelCm * calibrationScale + calibrationOffsetCm;
    if (out.calibratedLevelCm < 0.0f || out.calibratedLevelCm > maximumLevelCm) {
        out.status = SENSOR_MATH_LEVEL_OUT_OF_RANGE;
        return out;
    }

    out.status = SENSOR_MATH_OK;
    return out;
}

} // namespace HWC
