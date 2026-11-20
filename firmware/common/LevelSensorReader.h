#ifndef HWC_LEVEL_SENSOR_READER_H
#define HWC_LEVEL_SENSOR_READER_H

#include <Arduino.h>
#include "SensorConfig.h"
#include "SensorMath.h"

namespace HWC {

struct LevelSensorReading {
    bool adcReadOk;
    uint16_t millivolts;
    SensorMathResult result;
};

class LevelSensorReader {
public:
    LevelSensorReader(uint8_t pin, const SensorConfig& config);
    void begin();
    LevelSensorReading read();
    void setConfig(const SensorConfig& config);
private:
    uint8_t pin_;
    SensorConfig config_;
};

} // namespace HWC

#endif
