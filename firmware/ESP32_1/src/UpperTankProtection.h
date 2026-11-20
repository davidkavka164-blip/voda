#ifndef HWC_UPPER_TANK_PROTECTION_H
#define HWC_UPPER_TANK_PROTECTION_H
#include "UpperTankLimits.h"
namespace HWC {
struct UpperTankProtectionState {
    bool pumpPermit;
    bool criticalLevelLatched;
    bool startupValidationRequired;
    bool sensorWarning;
    bool systemFault;
    bool emergencyModeActive;
};
class UpperTankProtection {
public:
    explicit UpperTankProtection(bool savedCriticalLatch = false, bool savedStateValid = false);
    UpperTankProtectionState update(const UpperTankLimits& limits, float levelCm,
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
