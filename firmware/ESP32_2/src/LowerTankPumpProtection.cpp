#include "LowerTankPumpProtection.h"
#include <math.h>
namespace HWC {
namespace {
uint32_t crcByte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}
template <typename T> uint32_t crcValue(uint32_t crc, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) crc = crcByte(crc, bytes[i]);
    return crc;
}
}
LowerTankPumpConfig emptyLowerTankPumpConfig() {
    LowerTankPumpConfig c = {};
    c.magic = LOWER_PUMP_CONFIG_MAGIC;
    c.version = LOWER_PUMP_CONFIG_VERSION;
    updateLowerTankPumpConfigChecksum(c);
    return c;
}
bool isLowerTankPumpConfigValid(const LowerTankPumpConfig& c) {
    return c.magic == LOWER_PUMP_CONFIG_MAGIC && c.version == LOWER_PUMP_CONFIG_VERSION &&
           isfinite(c.minimumLevelCm) && c.minimumLevelCm >= 0.0f &&
           isfinite(c.hysteresisCm) && c.hysteresisCm > 0.0f &&
           c.minimumLevelCm + c.hysteresisCm <= LOWER_PUMP_TANK_MAX_LEVEL_CM;
}
uint32_t lowerTankPumpConfigChecksum(const LowerTankPumpConfig& c) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcValue(crc, c.magic); crc = crcValue(crc, c.version);
    crc = crcValue(crc, c.minimumLevelCm); crc = crcValue(crc, c.hysteresisCm);
    return crc ^ 0xFFFFFFFFUL;
}
bool lowerTankPumpConfigChecksumMatches(const LowerTankPumpConfig& c) {
    return c.checksum == lowerTankPumpConfigChecksum(c);
}
void updateLowerTankPumpConfigChecksum(LowerTankPumpConfig& c) {
    c.checksum = lowerTankPumpConfigChecksum(c);
}
LowerTankPumpProtection::LowerTankPumpProtection(bool savedCriticalLatch, bool savedStateValid)
    : criticalLatched_(savedStateValid && savedCriticalLatch),
      startupValidationRequired_(true), emergencyMode_(false) {}
LowerTankPumpState LowerTankPumpProtection::update(const LowerTankPumpConfig& config, float levelCm,
                                                    bool sensorValid, bool storageReady,
                                                    bool enableEmergencyCommand,
                                                    bool emergencyConfirmation,
                                                    bool disableEmergencyCommand) {
    const bool configValid = isLowerTankPumpConfigValid(config) && lowerTankPumpConfigChecksumMatches(config);
    const bool readingValid = sensorValid && isfinite(levelCm) && levelCm >= 0.0f &&
                              levelCm <= LOWER_PUMP_TANK_MAX_LEVEL_CM;
    if (enableEmergencyCommand && emergencyConfirmation) {
        // RAM-only operator override: physical level inspection replaces all software
        // level-protection checks, including a critical latch and unavailable config/NVS.
        emergencyMode_ = true;
        startupValidationRequired_ = false;
    }
    if (disableEmergencyCommand) {
        emergencyMode_ = false;
        if (!readingValid) startupValidationRequired_ = true;
    }
    if (configValid && readingValid) {
        const float recoveryLevel = config.minimumLevelCm + config.hysteresisCm;
        if (levelCm <= config.minimumLevelCm) criticalLatched_ = true;
        else if (levelCm >= recoveryLevel) {
            criticalLatched_ = false;
            startupValidationRequired_ = false;
        }
    } else if (!readingValid && !emergencyMode_) {
        startupValidationRequired_ = true;
    }
    const bool systemFault = !configValid || !storageReady;
    const bool normalPermit = !systemFault && !criticalLatched_ && !startupValidationRequired_ && readingValid;
    // Emergency mode overrides software-only faults; the relay hardware remains fail-off.
    const bool permit = emergencyMode_ || normalPermit;
    LowerTankPumpState out = {};
    out.pumpPermit = permit;
    out.criticalLevelLatched = criticalLatched_;
    out.startupValidationRequired = startupValidationRequired_;
    out.sensorWarning = !readingValid;
    out.systemFault = systemFault;
    out.emergencyModeActive = emergencyMode_;
    return out;
}
bool LowerTankPumpProtection::criticalLevelLatched() const { return criticalLatched_; }
bool LowerTankPumpProtection::emergencyModeActive() const { return emergencyMode_; }
}
