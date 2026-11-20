#include "NvsWellProtectionStore.h"
#include <cmath>
#include <string.h>

namespace HWC {
namespace {
static const uint32_t SLOT_MAGIC = 0x48575053UL; // HWPS
static const uint16_t SLOT_VERSION = 1;
struct SlotRecord { uint32_t magic; uint16_t version; uint16_t payloadSize; uint32_t generation; WellProtectionRecord payload; uint32_t checksum; };
uint32_t crcByte(uint32_t crc, uint8_t byte) { crc ^= byte; for (uint8_t bit=0; bit<8; ++bit) { const uint32_t mask=static_cast<uint32_t>(-(static_cast<int32_t>(crc&1U))); crc=(crc>>1)^(0xEDB88320UL&mask); } return crc; }
template<typename T> uint32_t crcValue(uint32_t crc,const T& value) { const uint8_t* p=reinterpret_cast<const uint8_t*>(&value); for(size_t i=0;i<sizeof(T);++i) crc=crcByte(crc,p[i]); return crc; }
bool basicConfigValid(const WellProtectionConfig& c) { return c.magic==WELL_PROTECTION_CONFIG_MAGIC && c.version==WELL_PROTECTION_CONFIG_VERSION && std::isfinite(c.transferMinimumLevelCm) && c.transferMinimumLevelCm>=0.0f && c.transferMinimumLevelCm<=2000.0f && std::isfinite(c.criticalTripLevelCm) && c.criticalTripLevelCm>=0.0f && c.criticalTripLevelCm<2000.0f && std::isfinite(c.recoveryLevelCm) && c.recoveryLevelCm>c.criticalTripLevelCm && c.recoveryLevelCm<=2000.0f && wellProtectionConfigChecksumMatches(c); }
uint32_t slotChecksum(const SlotRecord& r) {
    uint32_t c=0xFFFFFFFFUL;
    c=crcValue(c,r.magic); c=crcValue(c,r.version); c=crcValue(c,r.payloadSize); c=crcValue(c,r.generation);
    // Hash semantic payload fields only. Never include compiler padding bytes.
    c=crcValue(c,r.payload.magic); c=crcValue(c,r.payload.version);
    c=crcValue(c,r.payload.config.magic); c=crcValue(c,r.payload.config.version);
    c=crcValue(c,r.payload.config.transferMinimumLevelCm);
    c=crcValue(c,r.payload.config.criticalTripLevelCm);
    c=crcValue(c,r.payload.config.recoveryLevelCm);
    c=crcValue(c,r.payload.config.checksum);
    c=crcValue(c,r.payload.criticalLatch); c=crcValue(c,r.payload.safetyStateValid);
    c=crcValue(c,r.payload.checksum);
    return c^0xFFFFFFFFUL;
}
bool samePayload(const WellProtectionRecord& a,const WellProtectionRecord& b) {
    return a.magic==b.magic && a.version==b.version &&
      a.config.magic==b.config.magic && a.config.version==b.config.version &&
      a.config.transferMinimumLevelCm==b.config.transferMinimumLevelCm &&
      a.config.criticalTripLevelCm==b.config.criticalTripLevelCm &&
      a.config.recoveryLevelCm==b.config.recoveryLevelCm &&
      a.config.checksum==b.config.checksum && a.criticalLatch==b.criticalLatch &&
      a.safetyStateValid==b.safetyStateValid && a.checksum==b.checksum;
}
bool slotValid(const SlotRecord& r) { return r.magic==SLOT_MAGIC && r.version==SLOT_VERSION && r.payloadSize==sizeof(WellProtectionRecord) && r.generation!=0 && isWellProtectionRecordValid(r.payload) && r.checksum==slotChecksum(r); }
enum SlotStatus { SLOT_ABSENT, SLOT_VALID, SLOT_CORRUPT };
SlotStatus readSlot(Preferences& p,const char* key,SlotRecord& r) { if(!p.isKey(key)) return SLOT_ABSENT; if(p.getBytesLength(key)!=sizeof(r) || p.getBytes(key,&r,sizeof(r))!=sizeof(r) || !slotValid(r)) return SLOT_CORRUPT; return SLOT_VALID; }
bool writeSlot(Preferences& p,const char* key,const WellProtectionRecord& payload,uint32_t generation) { SlotRecord r={}; r.magic=SLOT_MAGIC; r.version=SLOT_VERSION; r.payloadSize=sizeof(payload); r.generation=generation; r.payload=payload; r.checksum=slotChecksum(r); if(p.putBytes(key,&r,sizeof(r))!=sizeof(r)) return false; SlotRecord verify={}; return readSlot(p,key,verify)==SLOT_VALID && verify.generation==generation && samePayload(verify.payload,payload); }
bool generationsAdjacent(uint32_t a,uint32_t b) {
    // Every successful save increments exactly once; generation zero is forbidden
    // and wrap is rejected by save(). A non-adjacent pair is therefore ambiguous.
    return (a > b && (a - b) == 1U) || (b > a && (b - a) == 1U);
}
bool newer(uint32_t a,uint32_t b) { return a > b; }
}
uint32_t wellProtectionRecordChecksum(const WellProtectionRecord& r) { uint32_t c=0xFFFFFFFFUL; c=crcValue(c,r.magic); c=crcValue(c,r.version); c=crcValue(c,r.config.magic); c=crcValue(c,r.config.version); c=crcValue(c,r.config.transferMinimumLevelCm); c=crcValue(c,r.config.criticalTripLevelCm); c=crcValue(c,r.config.recoveryLevelCm); c=crcValue(c,r.config.checksum); c=crcValue(c,r.criticalLatch); c=crcValue(c,r.safetyStateValid); return c^0xFFFFFFFFUL; }
bool isWellProtectionRecordValid(const WellProtectionRecord& r) { return r.magic==WELL_PROTECTION_RECORD_MAGIC && r.version==WELL_PROTECTION_RECORD_VERSION && r.criticalLatch<=1U && r.safetyStateValid<=1U && basicConfigValid(r.config) && r.checksum==wellProtectionRecordChecksum(r); }
NvsWellProtectionStore::NvsWellProtectionStore(const char* n):nameSpace_(n),opened_(false){}
bool NvsWellProtectionStore::begin(){ if(opened_)return true; opened_=nameSpace_&&nameSpace_[0]&&preferences_.begin(nameSpace_,false); return opened_; }
void NvsWellProtectionStore::end(){if(opened_)preferences_.end();opened_=false;}
bool NvsWellProtectionStore::hasRecord(){return opened_&&(preferences_.isKey("state")||preferences_.isKey("slot0")||preferences_.isKey("slot1"));}
bool NvsWellProtectionStore::load(WellProtectionConfig& config,bool& latch,bool& safetyValid){
 if (!opened_) return false;
 SlotRecord a={}, b={};
 SlotStatus sa=readSlot(preferences_,"slot0",a), sb=readSlot(preferences_,"slot1",b);
 if(sa==SLOT_CORRUPT||sb==SLOT_CORRUPT)return false;
 if(sa==SLOT_VALID||sb==SLOT_VALID){ const SlotRecord* chosen=nullptr; if(sa==SLOT_VALID&&sb==SLOT_VALID){ if(!generationsAdjacent(a.generation,b.generation))return false; chosen=newer(a.generation,b.generation)?&a:&b; } else chosen=sa==SLOT_VALID?&a:&b; config=chosen->payload.config; latch=chosen->payload.criticalLatch!=0; safetyValid=chosen->payload.safetyStateValid!=0; return true; }
 // No new slots: read legacy state without modifying it until a verified slot exists.
 if(!preferences_.isKey("state")||preferences_.getBytesLength("state")!=sizeof(WellProtectionRecord))return false;
 WellProtectionRecord legacy={}; if(preferences_.getBytes("state",&legacy,sizeof(legacy))!=sizeof(legacy)||!isWellProtectionRecordValid(legacy))return false;
 if(!writeSlot(preferences_,"slot0",legacy,1))return false;
 config=legacy.config; latch=legacy.criticalLatch!=0; safetyValid=legacy.safetyStateValid!=0; return true;
}
bool NvsWellProtectionStore::save(const WellProtectionConfig& config,bool latch,bool safetyValid){
 if(!opened_||!basicConfigValid(config))return false;
 WellProtectionRecord payload={}; payload.magic=WELL_PROTECTION_RECORD_MAGIC; payload.version=WELL_PROTECTION_RECORD_VERSION; payload.config=config; payload.criticalLatch=latch?1U:0U; payload.safetyStateValid=safetyValid?1U:0U; payload.checksum=wellProtectionRecordChecksum(payload);
 if(hasRecord()){ WellProtectionConfig old=emptyWellProtectionConfig(); bool oldLatch=false,oldValid=false; if(!load(old,oldLatch,oldValid))return false; }
 SlotRecord a={},b={}; SlotStatus sa=readSlot(preferences_,"slot0",a),sb=readSlot(preferences_,"slot1",b); if(sa==SLOT_CORRUPT||sb==SLOT_CORRUPT)return false;
 const char* target="slot0"; uint32_t generation=1;
 if(sa==SLOT_VALID&&sb==SLOT_VALID){ const bool aNewer=newer(a.generation,b.generation); const SlotRecord& newest=aNewer?a:b; generation=newest.generation+1U; if(generation==0)return false; target=aNewer?"slot1":"slot0"; }
 else if(sa==SLOT_VALID){generation=a.generation+1U;if(generation==0)return false;target="slot1";}
 else if(sb==SLOT_VALID){generation=b.generation+1U;if(generation==0)return false;target="slot0";}
 else if(preferences_.isKey("state")){ WellProtectionConfig old=emptyWellProtectionConfig(); bool oldLatch=false,oldValid=false; if(!load(old,oldLatch,oldValid))return false; // load migrates legacy into slot0
   sa=readSlot(preferences_,"slot0",a); if(sa!=SLOT_VALID)return false; generation=a.generation+1U;if(generation==0)return false;target="slot1"; }
 return writeSlot(preferences_,target,payload,generation);
}
bool NvsWellProtectionStore::saveCriticalLatch(bool latch){WellProtectionConfig c=emptyWellProtectionConfig();bool old=false,valid=false;if(!load(c,old,valid))return false;return save(c,latch,valid);}
} // namespace HWC
