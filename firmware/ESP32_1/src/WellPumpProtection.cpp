#include "WellPumpProtection.h"
#include <math.h>

namespace HWC {

WellPumpProtection::WellPumpProtection(bool savedCriticalLatch, bool savedSafetyStateValid)
    : criticalLatched_(savedSafetyStateValid && savedCriticalLatch), emergencyMode_(false), startupValidationRequired_(true) {}

PumpProtectionOutputs WellPumpProtection::update(const PumpProtectionInputs &inputs) {
    const bool thresholdsValid = inputs.configurationValid &&
        isfinite(inputs.criticalTripLevelCm) && isfinite(inputs.recoveryLevelCm) &&
        inputs.criticalTripLevelCm >= 0.0f && inputs.recoveryLevelCm > inputs.criticalTripLevelCm &&
        isfinite(inputs.maximumLevelCm) && inputs.maximumLevelCm > inputs.recoveryLevelCm;
    const bool sensorUsable = inputs.sensorValid && thresholdsValid &&
        isfinite(inputs.waterLevelCm) && inputs.waterLevelCm >= 0.0f &&
        inputs.waterLevelCm <= inputs.maximumLevelCm;

    if (inputs.enableEmergencyCommand && inputs.emergencyConfirmation) {
        // Emergency mode is an explicit, RAM-only operator override. The operator has
        // physically checked the water level and accepts responsibility for monitoring it.
        // It bypasses every software level-protection/configuration/storage latch, but never
        // bypasses the physical relay output readiness or independent motor protection.
        emergencyMode_ = true;
        startupValidationRequired_ = false;
    }
    if (inputs.disableEmergencyCommand) {
        emergencyMode_ = false;
        if (!sensorUsable) startupValidationRequired_ = true;
    }

    if (sensorUsable) {
        if (inputs.waterLevelCm <= inputs.criticalTripLevelCm) criticalLatched_ = true;
        else if (inputs.waterLevelCm >= inputs.recoveryLevelCm) {
            criticalLatched_ = false;
            startupValidationRequired_ = false;
        }
    }

    const bool systemFault = !thresholdsValid || inputs.otherCriticalFault;
    const bool normalPermit = !systemFault && !criticalLatched_ && !startupValidationRequired_ && sensorUsable;
    // Explicit physical inspection overrides software-only protection faults and latches.
    const bool permit = emergencyMode_ || normalPermit;

    PumpProtectionOutputs out;
    out.pumpPermit = permit;
    out.criticalLevelLatched = criticalLatched_;
    out.emergencyModeActive = emergencyMode_;
    out.sensorWarning = !sensorUsable;
    out.systemFault = systemFault;
    return out;
}

bool WellPumpProtection::emergencyModeActive() const { return emergencyMode_; }
bool WellPumpProtection::criticalLevelLatched() const { return criticalLatched_; }

} // namespace HWC
