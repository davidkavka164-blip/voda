#include "UpperTankProtection.h"
#include <math.h>
namespace HWC {
UpperTankProtection::UpperTankProtection(bool savedCriticalLatch, bool savedStateValid)
    : criticalLatched_(savedStateValid && savedCriticalLatch), startupValidationRequired_(true), emergencyMode_(false) {}
UpperTankProtectionState UpperTankProtection::update(const UpperTankLimits& limits, float levelCm,
                                                     bool sensorValid, bool storageReady,
                                                     bool enableEmergencyCommand,
                                                     bool emergencyConfirmation,
                                                     bool disableEmergencyCommand) {
    const bool configValid = isUpperTankLimitsValid(limits);
    const bool readingValid = sensorValid && isfinite(levelCm) && levelCm >= 0.0f &&
                              configValid && levelCm <= limits.usableDepthCm;
    if (enableEmergencyCommand && emergencyConfirmation) {
        // RAM-only operator override after physical level inspection. It bypasses
        // software level-protection/configuration/storage faults, not motor protection.
        emergencyMode_ = true;
        startupValidationRequired_ = false;
    }
    if (disableEmergencyCommand) {
        emergencyMode_ = false;
        // If the sensor is still invalid, normal protection remains blocked until recovery.
        if (!readingValid) startupValidationRequired_ = true;
    }

    if (!readingValid) {
        // Emergency confirmation is the only exception to the normal startup/sensor recovery gate.
        if (!emergencyMode_) startupValidationRequired_ = true;
    } else {
        if (levelCm <= limits.pumpCriticalTripCm) criticalLatched_ = true;
        else if (levelCm >= limits.pumpRecoveryCm) {
            criticalLatched_ = false;
            startupValidationRequired_ = false;
        }
    }
    const bool systemFault = !configValid || !storageReady;
    const bool normalPermit = !systemFault && !criticalLatched_ && !startupValidationRequired_ && readingValid;
    const bool permit = emergencyMode_ || normalPermit;
    UpperTankProtectionState state = {};
    state.pumpPermit = permit;
    state.criticalLevelLatched = criticalLatched_;
    state.startupValidationRequired = startupValidationRequired_;
    state.sensorWarning = !readingValid;
    state.systemFault = systemFault;
    state.emergencyModeActive = emergencyMode_;
    return state;
}
bool UpperTankProtection::criticalLevelLatched() const { return criticalLatched_; }
bool UpperTankProtection::emergencyModeActive() const { return emergencyMode_; }
}
