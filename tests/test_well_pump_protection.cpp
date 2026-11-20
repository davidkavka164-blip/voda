#include "../firmware/common/WellPumpProtection.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;
static PumpProtectionInputs goodInputs() {
    PumpProtectionInputs in = {};
    in.sensorValid = true; in.waterLevelCm = 100.0f;
    in.criticalTripLevelCm = 20.0f; in.recoveryLevelCm = 40.0f;
    in.maximumLevelCm = 800.0f; in.configurationValid = true;
    return in;
}
int main() {
    WellPumpProtection p;
    PumpProtectionInputs in = goodInputs();
    PumpProtectionOutputs out = p.update(in);
    assert(out.pumpPermit && !out.criticalLevelLatched);

    WellPumpProtection startup;
    PumpProtectionInputs si = goodInputs(); si.sensorValid = false;
    out = startup.update(si); assert(!out.pumpPermit);
    si.enableEmergencyCommand = true; si.emergencyConfirmation = false;
    out = startup.update(si); assert(!out.pumpPermit && !out.emergencyModeActive);
    si.emergencyConfirmation = true;
    out = startup.update(si); assert(out.pumpPermit && out.emergencyModeActive);

    in.waterLevelCm = 18.0f; out = p.update(in);
    assert(!out.pumpPermit && out.criticalLevelLatched);
    in.waterLevelCm = 30.0f; out = p.update(in);
    assert(!out.pumpPermit && out.criticalLevelLatched);
    // Physical inspection explicitly overrides a stored critical-low latch.
    in.enableEmergencyCommand = true; in.emergencyConfirmation = true;
    out = p.update(in); assert(out.pumpPermit && out.emergencyModeActive && out.criticalLevelLatched);
    in.disableEmergencyCommand = true; in.enableEmergencyCommand = false;
    out = p.update(in); assert(!out.pumpPermit && !out.emergencyModeActive && out.criticalLevelLatched);

    // Emergency override also works when sensor, configuration and NVS are unavailable.
    WellPumpProtection faults;
    in = goodInputs(); in.sensorValid = false; in.configurationValid = false;
    in.otherCriticalFault = true; in.enableEmergencyCommand = true; in.emergencyConfirmation = true;
    out = faults.update(in);
    assert(out.pumpPermit && out.emergencyModeActive && out.systemFault && out.sensorWarning);

    // A confirmed emergency can be selected even while a valid sensor reports a low level.
    WellPumpProtection lowLevel;
    in = goodInputs(); in.waterLevelCm = 10.0f; out = lowLevel.update(in);
    assert(!out.pumpPermit && out.criticalLevelLatched);
    in.enableEmergencyCommand = true; in.emergencyConfirmation = true;
    out = lowLevel.update(in); assert(out.pumpPermit && out.emergencyModeActive && out.criticalLevelLatched);

    // Emergency is RAM-only and is cleared by restart.
    WellPumpProtection rebooted(false, true);
    in = goodInputs(); in.sensorValid = false;
    out = rebooted.update(in); assert(!out.pumpPermit && !out.emergencyModeActive);
    puts("PASS: well pump emergency overrides sensor/config/NVS/critical latch only after physical confirmation; restart clears override");
}
