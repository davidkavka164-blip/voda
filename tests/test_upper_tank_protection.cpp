#include "../firmware/common/UpperTankProtection.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;
static UpperTankLimits config() { UpperTankLimits c=emptyUpperTankLimits(); c.usableDepthCm=120.0f; c.pumpCriticalTripCm=18.0f; c.pumpRecoveryCm=28.0f; c.householdReserveCm=35.0f; c.transferValveOpenCm=60.0f; updateUpperTankLimitsChecksum(c); return c; }
int main() {
    UpperTankLimits c=config(); UpperTankProtection p;
    UpperTankProtectionState s=p.update(c,23.0f,true,true); assert(!s.pumpPermit&&s.startupValidationRequired);
    s=p.update(c,28.0f,true,true); assert(s.pumpPermit&&!s.startupValidationRequired);
    s=p.update(c,18.0f,true,true); assert(!s.pumpPermit&&s.criticalLevelLatched);
    s=p.update(c,23.0f,true,true); assert(!s.pumpPermit&&s.criticalLevelLatched);
    // Confirmed physical inspection overrides a latched critical-low state.
    s=p.update(c,23.0f,true,true,true,true,false); assert(s.pumpPermit&&s.emergencyModeActive&&s.criticalLevelLatched);
    s=p.update(c,23.0f,true,true,false,false,true); assert(!s.pumpPermit&&!s.emergencyModeActive&&s.criticalLevelLatched);

    UpperTankProtection sensorFault;
    s=sensorFault.update(c,0.0f,false,true); assert(!s.pumpPermit&&s.sensorWarning);
    s=sensorFault.update(c,0.0f,false,true,true,false,false); assert(!s.pumpPermit&&!s.emergencyModeActive);
    s=sensorFault.update(c,0.0f,false,true,true,true,false); assert(s.pumpPermit&&s.emergencyModeActive);

    // Emergency is still available when limits, safety-state NVS or sensor are invalid.
    UpperTankLimits bad=c; bad.checksum^=1U;
    UpperTankProtection noStorage;
    s=noStorage.update(bad,0.0f,false,false,true,true,false);
    assert(s.pumpPermit&&s.emergencyModeActive&&s.systemFault);

    UpperTankProtection rebooted(true,true);
    s=rebooted.update(c,23.0f,true,true); assert(!s.pumpPermit&&s.criticalLevelLatched);
    UpperTankProtection rebootedAfterEmergency(false,true);
    s=rebootedAfterEmergency.update(c,50.0f,false,true); assert(!s.pumpPermit&&!s.emergencyModeActive&&s.sensorWarning);
    puts("PASS: upper pump emergency overrides sensor/config/NVS/critical latch only after physical confirmation; restart clears override");
}
