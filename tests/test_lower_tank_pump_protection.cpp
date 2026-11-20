#include "../firmware/common/LowerTankPumpProtection.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;
static LowerTankPumpConfig cfg(float minimum=15.0f,float hysteresis=5.0f){LowerTankPumpConfig c=emptyLowerTankPumpConfig();c.minimumLevelCm=minimum;c.hysteresisCm=hysteresis;updateLowerTankPumpConfigChecksum(c);return c;}
int main(){
    LowerTankPumpConfig c=cfg(); LowerTankPumpProtection p;
    LowerTankPumpState s=p.update(c,19.0f,true,true); assert(!s.pumpPermit&&s.startupValidationRequired);
    s=p.update(c,20.0f,true,true); assert(s.pumpPermit&&!s.startupValidationRequired);
    s=p.update(c,15.0f,true,true); assert(!s.pumpPermit&&s.criticalLevelLatched);
    s=p.update(c,18.0f,true,true); assert(!s.pumpPermit&&s.criticalLevelLatched);
    // Confirmed emergency overrides a critical latch while retaining its diagnostic state.
    s=p.update(c,18.0f,true,true,true,true); assert(s.pumpPermit&&s.emergencyModeActive&&s.criticalLevelLatched);
    s=p.update(c,18.0f,true,true,false,false,true); assert(!s.pumpPermit&&!s.emergencyModeActive&&s.criticalLevelLatched);

    LowerTankPumpProtection sensorFault;
    s=sensorFault.update(c,0.0f,false,true); assert(!s.pumpPermit&&s.sensorWarning);
    s=sensorFault.update(c,0.0f,false,true,true,false); assert(!s.pumpPermit&&!s.emergencyModeActive);
    s=sensorFault.update(c,0.0f,false,true,true,true); assert(s.pumpPermit&&s.emergencyModeActive);
    s=sensorFault.update(c,20.0f,true,true); assert(s.pumpPermit&&s.emergencyModeActive);

    // Config checksum error and unavailable NVS are software faults, not pump faults.
    LowerTankPumpProtection noStorage;
    s=noStorage.update(c,0.0f,false,false,true,true); assert(s.pumpPermit&&s.systemFault&&s.emergencyModeActive);
    LowerTankPumpConfig bad=cfg(); bad.checksum^=1U;
    LowerTankPumpProtection badConfig;
    s=badConfig.update(bad,50.0f,true,false,true,true); assert(s.pumpPermit&&s.systemFault&&s.emergencyModeActive);

    LowerTankPumpProtection rebooted(false,true);
    s=rebooted.update(c,0.0f,false,true); assert(!s.pumpPermit&&!s.emergencyModeActive&&s.sensorWarning);
    puts("PASS: lower pump emergency overrides sensor/config/NVS/critical latch only after physical confirmation; restart clears override");
}
