#include <assert.h>
#include <stdio.h>
#include <Preferences.h>
#include "NvsLowerTankPumpProtectionStore.h"
Preferences::Entry Preferences::entries[16] = {};
int32_t Preferences::writeLimit = -1;
size_t Preferences::failAtWriteCall = 0, Preferences::failBytes = 0, Preferences::writeCallCount = 0;
using namespace HWC;
static LowerTankPumpConfig cfg(float min,float hyst){LowerTankPumpConfig c=emptyLowerTankPumpConfig();c.minimumLevelCm=min;c.hysteresisCm=hyst;updateLowerTankPumpConfigChecksum(c);return c;}
int main(){
 Preferences::clearForTest(); NvsLowerTankPumpProtectionStore s("hwc_lowpump");assert(s.begin());assert(!s.hasRecord());
 LowerTankPumpConfig c=cfg(15,5);assert(s.save(c,false,true));assert(s.hasRecord());
 LowerTankPumpConfig loaded={};bool latch=true,valid=false;assert(s.load(loaded,latch,valid));assert(!latch&&valid&&loaded.minimumLevelCm==15&&loaded.hysteresisCm==5);
 assert(s.saveCriticalLatch(true));assert(s.load(loaded,latch,valid)&&latch);
 LowerTankPumpConfig changed=cfg(18,7);assert(s.save(changed,true,true));assert(s.load(loaded,latch,valid));assert(latch&&loaded.minimumLevelCm==18&&loaded.hysteresisCm==7);
 // Partial write to inactive slot must not destroy the previous valid slot.
 Preferences::clearForTest();NvsLowerTankPumpProtectionStore cut("hwc_lowpump");assert(cut.begin());assert(cut.save(c,false,true));
 Preferences::simulatePartialWriteAfter(3);assert(!cut.save(changed,false,true));Preferences::clearWriteFailure();
 LowerTankPumpConfig after={};bool afterLatch=false,afterValid=false;assert(!cut.load(after,afterLatch,afterValid)); // damaged slot + valid previous slot is deliberately treated as ambiguous/corrupt
 bool intact=false;for(size_t i=0;i<16;++i)if(Preferences::entries[i].used&&Preferences::entries[i].length>0)intact=true;assert(intact);
 puts("PASS: lower tank pump NVS persistence, checksum, latch update and interrupted-write fail-closed behavior");
}
