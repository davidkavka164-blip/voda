#ifndef HWC_LOWER_TANK_PUMP_PROTECTION_H
#define HWC_LOWER_TANK_PUMP_PROTECTION_H
#include <stdint.h>
namespace HWC {
static const uint32_t LOWER_PUMP_CONFIG_MAGIC = 0x484C5043UL; // HLPC
static const uint16_t LOWER_PUMP_CONFIG_VERSION = 1;
static const float LOWER_PUMP_TANK_MAX_LEVEL_CM = 121.0f;
struct LowerTankPumpConfig {
    uint32_t magic;
    uint16_t version;
    float minimumLevelCm;
    float hysteresisCm;
    uint32_t checksum;
};
LowerTankPumpConfig emptyLowerTankPumpConfig();
bool isLowerTankPumpConfigValid(const LowerTankPumpConfig& config);
uint32_t lowerTankPumpConfigChecksum(const LowerTankPumpConfig& config);
bool lowerTankPumpConfigChecksumMatches(const LowerTankPumpConfig& config);
void updateLowerTankPumpConfigChecksum(LowerTankPumpConfig& config);
struct LowerTankPumpState {
    bool pumpPermit;
    bool criticalLevelLatched;
    bool startupValidationRequired;
    bool sensorWarning;
    bool systemFault;
    bool emergencyModeActive;
};
class LowerTankPumpProtection {
public:
    explicit LowerTankPumpProtection(bool savedCriticalLatch = false, bool savedStateValid = false);
    LowerTankPumpState update(const LowerTankPumpConfig& config, float levelCm,
                              bool sensorValid, bool storageReady,
                              bool enableEmergencyCommand = false,
                              bool emergencyConfirmation = false,
                              bool disableEmergencyCommand = false);
    bool criticalLevelLatched() const;
    bool emergencyModeActive() const;
private:
    bool criticalLatched_;
    bool startupValidationRequired_;
    bool emergencyMode_;
};
}
#endif
