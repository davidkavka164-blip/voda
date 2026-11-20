#include "UpperTankLimits.h"
#include <assert.h>
#include <stdio.h>
int main(){using namespace HWC; UpperTankLimits c=emptyUpperTankLimits(); c.usableDepthCm=120; c.pumpCriticalTripCm=18; c.pumpRecoveryCm=28; c.householdReserveCm=35; c.transferValveOpenCm=60; updateUpperTankLimitsChecksum(c); assert(isUpperTankLimitsValid(c));
 UpperTankValveDecision d=evaluateUpperTankTransferValve(c,59,true,false); assert(d.valid&&!d.permit&&!d.latchedOpen);
 d=evaluateUpperTankTransferValve(c,60,true,false); assert(d.valid&&d.permit&&d.latchedOpen);
 d=evaluateUpperTankTransferValve(c,45,true,true); assert(d.valid&&d.permit&&d.latchedOpen);
 d=evaluateUpperTankTransferValve(c,35,true,true); assert(d.valid&&!d.permit&&!d.latchedOpen);
 d=evaluateUpperTankTransferValve(c,80,false,true); assert(!d.valid&&!d.permit&&!d.latchedOpen);
 d=evaluateUpperTankTransferValve(c,121,true,true); assert(!d.valid&&!d.permit&&!d.latchedOpen);
 c.transferValveOpenCm=30; updateUpperTankLimitsChecksum(c); assert(!isUpperTankLimitsValid(c));
 c.transferValveOpenCm=60; c.pumpRecoveryCm=10; updateUpperTankLimitsChecksum(c); assert(!isUpperTankLimitsValid(c));
 puts("PASS: upper tank limit validation and reserve-valve hysteresis"); return 0;}
