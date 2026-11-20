#include "SensorConfig.h"
#include <math.h>
#include <stddef.h>

namespace HWC {
namespace {
uint32_t crc32Byte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}

uint32_t crc32Bytes(uint32_t crc, const void* data, size_t length) {
    const uint8_t* bytes = static_cast<const uint8_t*>(data);
    for (size_t i = 0; i < length; ++i) crc = crc32Byte(crc, bytes[i]);
    return crc;
}
}

SensorConfig defaultSensorConfig() {
    SensorConfig c = {};
    c.magic = SENSOR_CONFIG_MAGIC;
    c.version = SENSOR_CONFIG_VERSION;
    c.shuntOhms = 150.0f;
    c.currentMinMilliAmps = 4.0f;
    c.currentMaxMilliAmps = 20.0f;
    c.currentToleranceMilliAmps = 0.5f;
    c.sensorRangeCm = 1000.0f;
    c.calibrationScale = 1.0f;
    c.calibrationOffsetCm = 0.0f;
    updateSensorConfigChecksum(c);
    return c;
}

bool buildSensorConfigRecoveryCandidate(float scale, float rawLevelCm,
                                        float referenceLevelCm, SensorConfig& out) {
    if (!isfinite(scale) || scale < 0.5f || scale > 1.5f ||
        !isfinite(rawLevelCm) || rawLevelCm < 0.0f || rawLevelCm > 2000.0f ||
        !isfinite(referenceLevelCm) || referenceLevelCm < 0.0f || referenceLevelCm > 2000.0f) {
        return false;
    }

    SensorConfig candidate = defaultSensorConfig();
    candidate.calibrationScale = scale;
    candidate.calibrationOffsetCm = referenceLevelCm - (rawLevelCm * scale);
    updateSensorConfigChecksum(candidate);
    if (!isSensorConfigValid(candidate) || !sensorConfigChecksumMatches(candidate)) return false;
    out = candidate;
    return true;
}

bool isSensorConfigValid(const SensorConfig& c) {
    return c.magic == SENSOR_CONFIG_MAGIC &&
           c.version == SENSOR_CONFIG_VERSION &&
           isfinite(c.shuntOhms) && c.shuntOhms >= 50.0f && c.shuntOhms <= 250.0f &&
           isfinite(c.currentMinMilliAmps) && c.currentMinMilliAmps >= 3.0f && c.currentMinMilliAmps <= 5.0f &&
           isfinite(c.currentMaxMilliAmps) && c.currentMaxMilliAmps >= 18.0f && c.currentMaxMilliAmps <= 22.0f &&
           c.currentMaxMilliAmps > c.currentMinMilliAmps &&
           isfinite(c.currentToleranceMilliAmps) && c.currentToleranceMilliAmps >= 0.0f && c.currentToleranceMilliAmps <= 2.0f &&
           isfinite(c.sensorRangeCm) && c.sensorRangeCm >= 10.0f && c.sensorRangeCm <= 2000.0f &&
           isfinite(c.calibrationScale) && c.calibrationScale >= 0.5f && c.calibrationScale <= 1.5f &&
           isfinite(c.calibrationOffsetCm) && c.calibrationOffsetCm >= -2000.0f && c.calibrationOffsetCm <= 2000.0f;
}

uint32_t sensorConfigChecksum(const SensorConfig& c) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crc32Bytes(crc, &c.magic, sizeof(c.magic));
    crc = crc32Bytes(crc, &c.version, sizeof(c.version));
    crc = crc32Bytes(crc, &c.shuntOhms, sizeof(c.shuntOhms));
    crc = crc32Bytes(crc, &c.currentMinMilliAmps, sizeof(c.currentMinMilliAmps));
    crc = crc32Bytes(crc, &c.currentMaxMilliAmps, sizeof(c.currentMaxMilliAmps));
    crc = crc32Bytes(crc, &c.currentToleranceMilliAmps, sizeof(c.currentToleranceMilliAmps));
    crc = crc32Bytes(crc, &c.sensorRangeCm, sizeof(c.sensorRangeCm));
    crc = crc32Bytes(crc, &c.calibrationScale, sizeof(c.calibrationScale));
    crc = crc32Bytes(crc, &c.calibrationOffsetCm, sizeof(c.calibrationOffsetCm));
    return crc ^ 0xFFFFFFFFUL;
}

bool sensorConfigChecksumMatches(const SensorConfig& c) {
    return c.checksum == sensorConfigChecksum(c);
}

void updateSensorConfigChecksum(SensorConfig& c) {
    c.checksum = sensorConfigChecksum(c);
}

} // namespace HWC
