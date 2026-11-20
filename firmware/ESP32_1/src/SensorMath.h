#ifndef HWC_SENSOR_MATH_H
#define HWC_SENSOR_MATH_H

namespace HWC {

enum SensorMathStatus {
    SENSOR_MATH_OK,
    SENSOR_MATH_INVALID_INPUT,
    SENSOR_MATH_CURRENT_OUT_OF_RANGE,
    SENSOR_MATH_LEVEL_OUT_OF_RANGE
};

struct SensorMathResult {
    SensorMathStatus status;
    float currentMilliAmps;
    float rawLevelCm;
    float calibratedLevelCm;
};

SensorMathResult levelFromShuntMilliVolts(float shuntMilliVolts,
                                          float shuntOhms,
                                          float currentMinMilliAmps,
                                          float currentMaxMilliAmps,
                                          float currentToleranceMilliAmps,
                                          float sensorRangeCm,
                                          float calibrationScale,
                                          float calibrationOffsetCm,
                                          float maximumLevelCm);

} // namespace HWC

#endif
