#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <Preferences.h>
#include "NvsUpperTankLimitsStore.h"
Preferences::Entry Preferences::entries[16]={}; int32_t Preferences::writeLimit=-1;
size_t Preferences::failAtWriteCall=0, Preferences::failBytes=0, Preferences::writeCallCount=0;
namespace {
struct Legacy {uint32_t magic;uint16_t version;HWC::UpperTankLimits limits;uint32_t crc;};
uint32_t cb(uint32_t c,uint8_t b){c^=b;for(uint8_t i=0;i<8;++i){uint32_t m=static_cast<uint32_t>(-(static_cast<int32_t>(c&1U)));c=(c>>1)^(0xEDB88320UL&m);}return c;}
template<class T>uint32_t cv(uint32_t c,const T&v){const uint8_t*p=reinterpret_cast<const uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i)c=cb(c,p[i]);return c;}
uint32_t legacyC(const Legacy&r){uint32_t c=0xFFFFFFFFUL;c=cv(c,r.magic);c=cv(c,r.version);c=cv(c,r.limits.magic);c=cv(c,r.limits.version);c=cv(c,r.limits.usableDepthCm);c=cv(c,r.limits.pumpCriticalTripCm);c=cv(c,r.limits.pumpRecoveryCm);c=cv(c,r.limits.householdReserveCm);c=cv(c,r.limits.transferValveOpenCm);c=cv(c,r.limits.checksum);return c^0xFFFFFFFFUL;}
HWC::UpperTankLimits make(float d,float t,float r,float h,float v){HWC::UpperTankLimits x=HWC::emptyUpperTankLimits();x.usableDepthCm=d;x.pumpCriticalTripCm=t;x.pumpRecoveryCm=r;x.householdReserveCm=h;x.transferValveOpenCm=v;HWC::updateUpperTankLimitsChecksum(x);assert(HWC::isUpperTankLimitsValid(x));return x;}
const Preferences::Entry* ent(const char*k){for(size_t i=0;i<16;++i)if(Preferences::entries[i].used&&!strcmp(Preferences::entries[i].key,k))return &Preferences::entries[i];return nullptr;}
}
int main(){using namespace HWC;UpperTankLimits a=make(120,18,28,35,60),b=make(125,17,27,34,58),c=make(130,16,26,33,57),got={};
 Preferences::clearForTest();NvsUpperTankLimitsStore s("hwc_upperlimits");assert(s.begin()&&!s.hasRecord());assert(s.save(a)&&s.load(got)&&got.checksum==a.checksum);assert(ent("head")&&!ent("txn"));assert(s.save(b)&&s.load(got)&&got.checksum==b.checksum);
 Preferences::simulatePartialWriteOnCall(2,3);assert(!s.save(c));Preferences::clearWriteFailure();assert(s.load(got)&&got.checksum==b.checksum);assert(!ent("txn"));
 Preferences::simulatePartialWriteOnCall(3,2);assert(!s.save(c));Preferences::clearWriteFailure();assert(s.load(got)&&got.checksum==c.checksum);assert(!ent("txn"));
 // A torn txn marker is written before the candidate slot, so a valid head
 // still identifies the last committed record and can be safely used.
 Preferences::simulatePartialWriteOnCall(1,2);assert(!s.save(a));Preferences::clearWriteFailure();assert(s.load(got)&&got.checksum==c.checksum);assert(s.save(a)&&s.load(got)&&got.checksum==a.checksum);
 Preferences::Entry* inactive=nullptr;for(size_t i=0;i<16;++i)if(Preferences::entries[i].used&&!strcmp(Preferences::entries[i].key,"slot0"))inactive=&Preferences::entries[i];assert(inactive);inactive->bytes[0]^=0x40;assert(s.load(got)&&got.checksum==a.checksum);
 Preferences::Entry* head=nullptr;for(size_t i=0;i<16;++i)if(Preferences::entries[i].used&&!strcmp(Preferences::entries[i].key,"head"))head=&Preferences::entries[i];assert(head);head->bytes[0]^=1;assert(!s.load(got));s.end();
 // Interrupted first-ever initialization is recoverable only through a new explicit save.
 Preferences::clearForTest();NvsUpperTankLimitsStore init("hwc_upperlimits");assert(init.begin());Preferences::simulatePartialWriteOnCall(2,2);assert(!init.save(a));Preferences::clearWriteFailure();assert(!init.load(got));assert(init.save(b)&&init.load(got)&&got.checksum==b.checksum);init.end();
 // Legacy blob is validated and migrated without deleting the old key.
 Preferences::clearForTest();Legacy old={0x48555452UL,1,a,0};old.crc=legacyC(old);Preferences lp;assert(lp.begin("hwc_upperlimits",false));assert(lp.putBytes("state",&old,sizeof(old))==sizeof(old));lp.end();NvsUpperTankLimitsStore mig("hwc_upperlimits");assert(mig.begin()&&mig.load(got)&&got.checksum==a.checksum);assert(ent("state")&&ent("slot0")&&ent("head")&&!ent("txn"));mig.end();
 // Interrupted migration retries from untouched valid legacy bytes.
 Preferences::clearForTest();Preferences rp;assert(rp.begin("hwc_upperlimits",false));assert(rp.putBytes("state",&old,sizeof(old))==sizeof(old));rp.end();NvsUpperTankLimitsStore retry("hwc_upperlimits");assert(retry.begin());Preferences::simulatePartialWriteOnCall(2,2);assert(!retry.load(got));Preferences::clearWriteFailure();assert(ent("txn")&&ent("state"));assert(retry.load(got)&&got.checksum==a.checksum);assert(ent("head")&&!ent("txn"));retry.end();
 // Corrupt legacy state is never treated as a fresh install.
 Preferences::clearForTest();old.crc^=1;Preferences bp;assert(bp.begin("hwc_upperlimits",false));assert(bp.putBytes("state",&old,sizeof(old))==sizeof(old));bp.end();NvsUpperTankLimitsStore bad("hwc_upperlimits");assert(bad.begin()&&!bad.load(got)&&!bad.save(b));bad.end();
 puts("PASS: upper tank limits transactional two-slot NVS, migration and power-cut recovery");return 0;}
