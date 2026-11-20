#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <Preferences.h>
#include "NvsUpperTankProtectionStore.h"
Preferences::Entry Preferences::entries[16] = {};
int32_t Preferences::writeLimit = -1;
size_t Preferences::failAtWriteCall = 0, Preferences::failBytes = 0, Preferences::writeCallCount = 0;
namespace {
struct LegacyRecord { uint32_t magic; uint16_t version; uint8_t criticalLatch; uint8_t stateValid; uint32_t checksum; };
uint32_t crcByte(uint32_t c,uint8_t b){c^=b;for(uint8_t i=0;i<8;++i){const uint32_t m=static_cast<uint32_t>(-(static_cast<int32_t>(c&1U)));c=(c>>1)^(0xEDB88320UL&m);}return c;}
template<typename T>uint32_t crcValue(uint32_t c,const T& v){const uint8_t* p=reinterpret_cast<const uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i)c=crcByte(c,p[i]);return c;}
uint32_t legacyChecksum(const LegacyRecord& r){uint32_t c=0xFFFFFFFFUL;c=crcValue(c,r.magic);c=crcValue(c,r.version);c=crcValue(c,r.criticalLatch);c=crcValue(c,r.stateValid);return c^0xFFFFFFFFUL;}
}
int main(){
 Preferences::clearForTest();
 HWC::NvsUpperTankProtectionStore store("hwc_up_safety");
 assert(store.begin()); assert(!store.hasRecord());
 HWC::UpperTankSafetyState state={false,false}; assert(!store.load(state));
 assert(store.save(true,true)); assert(store.hasRecord());
 assert(store.load(state)); assert(state.criticalLatch&&state.stateValid);
 assert(store.save(false,true)); assert(store.load(state)); assert(!state.criticalLatch&&state.stateValid);
 // Any corrupt slot is fail-closed, even if the other slot is valid.
 Preferences::entries[0].bytes[0]^=0x5A;
 assert(!store.load(state)); assert(!store.save(false,true));
 store.end();

 // A partial write damages only the inactive slot and does not overwrite the
 // previous slot. Strict recovery refuses an ambiguous pair.
 Preferences::clearForTest();
 HWC::NvsUpperTankProtectionStore interrupted("hwc_up_safety");
 assert(interrupted.begin()); assert(interrupted.save(true,true));
 uint8_t previousSlot[256]={}; memcpy(previousSlot,Preferences::entries[0].bytes,Preferences::entries[0].length);
 const size_t previousLength=Preferences::entries[0].length;
 Preferences::simulatePartialWriteAfter(2);
 assert(!interrupted.save(false,true)); Preferences::clearWriteFailure();
 HWC::UpperTankSafetyState afterCut={false,false};
 assert(interrupted.hasRecord()); assert(!interrupted.load(afterCut));
 assert(Preferences::entries[0].length==previousLength);
 assert(memcmp(previousSlot,Preferences::entries[0].bytes,previousLength)==0);
 interrupted.end();

 // Reusing an existing older slot cannot damage the newest slot on interruption.
 Preferences::clearForTest();
 HWC::NvsUpperTankProtectionStore reuse("hwc_up_safety");
 assert(reuse.begin()); assert(reuse.save(true,true)); assert(reuse.save(false,true)); assert(reuse.save(true,true));
 Preferences::Entry newestUpper={};
 for(size_t i=0;i<16;++i) if(Preferences::entries[i].used && strcmp(Preferences::entries[i].key,"slot0")==0) newestUpper=Preferences::entries[i];
 assert(newestUpper.used);
 Preferences::simulatePartialWriteAfter(5); assert(!reuse.save(false,true)); Preferences::clearWriteFailure();
 HWC::UpperTankSafetyState afterReuse={false,false}; assert(!reuse.load(afterReuse));
 bool newestUpperPreserved=false;
 for(size_t i=0;i<16;++i){const Preferences::Entry& e=Preferences::entries[i];if(e.used&&strcmp(e.key,"slot0")==0&&e.length==newestUpper.length&&memcmp(e.bytes,newestUpper.bytes,newestUpper.length)==0)newestUpperPreserved=true;}
 assert(newestUpperPreserved); reuse.end();

 // Valid legacy state migrates to slot0 and is retained unchanged.
 Preferences::clearForTest();
 LegacyRecord legacy={0x48555346UL,1,1,1,0}; legacy.checksum=legacyChecksum(legacy);
 Preferences legacyPrefs; assert(legacyPrefs.begin("hwc_up_safety",false));
 assert(legacyPrefs.putBytes("state",&legacy,sizeof(legacy))==sizeof(legacy)); legacyPrefs.end();
 HWC::NvsUpperTankProtectionStore migrated("hwc_up_safety");
 assert(migrated.begin()&&migrated.hasRecord());
 HWC::UpperTankSafetyState migratedState={false,false};
 assert(migrated.load(migratedState)); assert(migratedState.criticalLatch&&migratedState.stateValid);
 assert(migrated.hasRecord()); migrated.end();

 // Malformed legacy record is never treated as a fresh installation.
 Preferences::clearForTest(); legacy.checksum^=1U;
 Preferences badLegacy; assert(badLegacy.begin("hwc_up_safety",false));
 assert(badLegacy.putBytes("state",&legacy,sizeof(legacy))==sizeof(legacy)); badLegacy.end();
 HWC::NvsUpperTankProtectionStore rejected("hwc_up_safety");
 assert(rejected.begin()&&rejected.hasRecord());
 assert(!rejected.load(migratedState)); assert(!rejected.save(false,true)); rejected.end();
 printf("PASS: upper tank protection two-slot NVS, migration, CRC and partial-write fail-closed tests\n");
 return 0;
}
