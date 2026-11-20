#ifndef HWC_WELL_PUMP_PROTECTION_H
#define HWC_WELL_PUMP_PROTECTION_H

namespace HWC {

struct PumpProtectionInputs {
    bool sensorValid;
    float waterLevelCm;
    float criticalTripLevelCm;
    float recoveryLevelCm;
    float maximumLevelCm;
    bool configurationValid;
    bool otherCriticalFault;
    bool enableEmergencyCommand;
    bool emergencyConfirmation;
    bool disableEmergencyCommand;
};

struct PumpProtectionOutputs {
    bool pumpPermit;
    bool criticalLevelLatched;
    bool emergencyModeActive;
    bool sensorWarning;
    bool systemFault;
};

class WellPumpProtection {
public:
    WellPumpProtection(bool savedCriticalLatch = false, bool savedSafetyStateValid = false);
    PumpProtectionOutputs update(const PumpProtectionInputs &inputs);
    bool emergencyModeActive() const;
    bool criticalLevelLatched() const;

private:
    bool criticalLatched_;
    bool emergencyMode_;
    bool startupValidationRequired_;
};

} // namespace HWC

#endif
