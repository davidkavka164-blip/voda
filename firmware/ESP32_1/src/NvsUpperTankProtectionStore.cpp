#include "NvsUpperTankProtectionStore.h"
#include <string.h>
namespace HWC { namespace {
static const uint32_t RECORD_MAGIC=0x48555346UL, SLOT_MAGIC=0x48555353UL;
static const uint16_t RECORD_VERSION=1, SLOT_VERSION=1;
struct LegacyRecord { uint32_t magic; uint16_t version; uint8_t criticalLatch; uint8_t stateValid; uint32_t checksum; };
struct SlotRecord { uint32_t magic; uint16_t version; uint16_t payloadSize; uint32_t generation; LegacyRecord payload; uint32_t checksum; };
uint32_t crcByte(uint32_t c,uint8_t b){c^=b;for(uint8_t i=0;i<8;++i){const uint32_t m=static_cast<uint32_t>(-(static_cast<int32_t>(c&1U)));c=(c>>1)^(0xEDB88320UL&m);}return c;}
template<typename T> uint32_t crcValue(uint32_t c,const T& v){const uint8_t* p=reinterpret_cast<const uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i)c=crcByte(c,p[i]);return c;}
uint32_t payloadChecksum(const LegacyRecord& r){uint32_t c=0xFFFFFFFFUL;c=crcValue(c,r.magic);c=crcValue(c,r.version);c=crcValue(c,r.criticalLatch);c=crcValue(c,r.stateValid);return c^0xFFFFFFFFUL;}
bool payloadValid(const LegacyRecord& r){return r.magic==RECORD_MAGIC&&r.version==RECORD_VERSION&&r.criticalLatch<=1&&r.stateValid==1&&r.checksum==payloadChecksum(r);}
uint32_t slotChecksum(const SlotRecord& r){
 uint32_t c=0xFFFFFFFFUL;
 c=crcValue(c,r.magic);c=crcValue(c,r.version);c=crcValue(c,r.payloadSize);c=crcValue(c,r.generation);
 // Hash semantic legacy payload fields, not compiler padding bytes.
 c=crcValue(c,r.payload.magic);c=crcValue(c,r.payload.version);
 c=crcValue(c,r.payload.criticalLatch);c=crcValue(c,r.payload.stateValid);c=crcValue(c,r.payload.checksum);
 return c^0xFFFFFFFFUL;
}
bool samePayload(const LegacyRecord& a,const LegacyRecord& b){
 return a.magic==b.magic&&a.version==b.version&&a.criticalLatch==b.criticalLatch&&
        a.stateValid==b.stateValid&&a.checksum==b.checksum;
}
bool slotValid(const SlotRecord& r){return r.magic==SLOT_MAGIC&&r.version==SLOT_VERSION&&r.payloadSize==sizeof(LegacyRecord)&&r.generation!=0&&payloadValid(r.payload)&&r.checksum==slotChecksum(r);}
enum Status{ABSENT,VALID,CORRUPT};
Status readSlot(Preferences& p,const char* key,SlotRecord& r){if(!p.isKey(key))return ABSENT;if(p.getBytesLength(key)!=sizeof(r)||p.getBytes(key,&r,sizeof(r))!=sizeof(r)||!slotValid(r))return CORRUPT;return VALID;}
bool writeSlot(Preferences& p,const char* key,const LegacyRecord& payload,uint32_t generation){SlotRecord r={};r.magic=SLOT_MAGIC;r.version=SLOT_VERSION;r.payloadSize=sizeof(payload);r.generation=generation;r.payload=payload;r.checksum=slotChecksum(r);if(p.putBytes(key,&r,sizeof(r))!=sizeof(r))return false;SlotRecord v={};return readSlot(p,key,v)==VALID&&v.generation==generation&&samePayload(v.payload,payload);}
bool generationsAdjacent(uint32_t a,uint32_t b){
 // Successful writes are consecutive; generation zero and wrap are forbidden.
 return (a>b&&(a-b)==1U)||(b>a&&(b-a)==1U);
}
bool newer(uint32_t a,uint32_t b){return a>b;}
} 
NvsUpperTankProtectionStore::NvsUpperTankProtectionStore(const char* n):nameSpace_(n),opened_(false){}
bool NvsUpperTankProtectionStore::begin(){if(opened_)return true;opened_=nameSpace_&&nameSpace_[0]&&preferences_.begin(nameSpace_,false);return opened_;}
void NvsUpperTankProtectionStore::end(){if(opened_)preferences_.end();opened_=false;}
bool NvsUpperTankProtectionStore::hasRecord(){return opened_&&(preferences_.isKey("state")||preferences_.isKey("slot0")||preferences_.isKey("slot1"));}
bool NvsUpperTankProtectionStore::load(UpperTankSafetyState& state){if(!opened_)return false;SlotRecord a={},b={};Status sa=readSlot(preferences_,"slot0",a),sb=readSlot(preferences_,"slot1",b);if(sa==CORRUPT||sb==CORRUPT)return false;
 LegacyRecord migrated={}; const LegacyRecord* p=nullptr;if(sa==VALID&&sb==VALID){if(!generationsAdjacent(a.generation,b.generation))return false;p=newer(a.generation,b.generation)?&a.payload:&b.payload;}else if(sa==VALID)p=&a.payload;else if(sb==VALID)p=&b.payload;
 if(!p){if(!preferences_.isKey("state")||preferences_.getBytesLength("state")!=sizeof(LegacyRecord))return false;if(preferences_.getBytes("state",&migrated,sizeof(migrated))!=sizeof(migrated)||!payloadValid(migrated))return false;if(!writeSlot(preferences_,"slot0",migrated,1))return false;p=&migrated;}
 state.criticalLatch=p->criticalLatch!=0;state.stateValid=p->stateValid==1;return state.stateValid;}
bool NvsUpperTankProtectionStore::save(bool latch,bool valid){if(!opened_||!valid)return false;if(hasRecord()){UpperTankSafetyState old={false,false};if(!load(old))return false;}
 LegacyRecord payload={};payload.magic=RECORD_MAGIC;payload.version=RECORD_VERSION;payload.criticalLatch=latch?1U:0U;payload.stateValid=1;payload.checksum=payloadChecksum(payload);
 SlotRecord a={},b={};Status sa=readSlot(preferences_,"slot0",a),sb=readSlot(preferences_,"slot1",b);if(sa==CORRUPT||sb==CORRUPT)return false;const char* target="slot0";uint32_t generation=1;
 if(sa==VALID&&sb==VALID){bool aNewer=newer(a.generation,b.generation);const SlotRecord& n=aNewer?a:b;generation=n.generation+1U;if(!generation)return false;target=aNewer?"slot1":"slot0";}
 else if(sa==VALID){generation=a.generation+1U;if(!generation)return false;target="slot1";}
 else if(sb==VALID){generation=b.generation+1U;if(!generation)return false;target="slot0";}
 else if(preferences_.isKey("state")){UpperTankSafetyState old={false,false};if(!load(old))return false;sa=readSlot(preferences_,"slot0",a);if(sa!=VALID)return false;generation=a.generation+1U;if(!generation)return false;target="slot1";}
 return writeSlot(preferences_,target,payload,generation);}
}
