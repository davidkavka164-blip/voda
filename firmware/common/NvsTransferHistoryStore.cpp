#include "NvsTransferHistoryStore.h"
namespace HWC {
namespace {
bool sameEntry(const TransferHistoryEntry& a, const TransferHistoryEntry& b) {
 return a.requestId==b.requestId && a.source==b.source &&
        a.terminalState==b.terminalState && a.endReason==b.endReason &&
        a.reserved==b.reserved && a.requestedLiters==b.requestedLiters &&
        a.estimatedTransferredLiters==b.estimatedTransferredLiters &&
        a.targetLowerLevelCm==b.targetLowerLevelCm &&
        a.finishedAtUptimeMs==b.finishedAtUptimeMs;
}
bool sameSnapshot(const TransferHistorySnapshot& a, const TransferHistorySnapshot& b) {
 if(a.count!=b.count || a.nextIndex!=b.nextIndex ||
    a.reserved[0]!=b.reserved[0] || a.reserved[1]!=b.reserved[1]) return false;
 for(size_t i=0;i<TRANSFER_HISTORY_CAPACITY;++i)
  if(!sameEntry(a.entries[i],b.entries[i])) return false;
 return true;
}
uint32_t crcByte(uint32_t c, uint8_t b) { c ^= b; for (uint8_t i=0;i<8;++i) { uint32_t m=static_cast<uint32_t>(-(static_cast<int32_t>(c&1U))); c=(c>>1)^(0xEDB88320UL&m); } return c; }
template<typename T> uint32_t crcVal(uint32_t c,const T& v) { const uint8_t* p=reinterpret_cast<const uint8_t*>(&v); for(size_t i=0;i<sizeof(T);++i)c=crcByte(c,p[i]); return c; }
}
uint32_t transferHistoryRecordChecksum(const TransferHistoryRecord& r) {
 uint32_t c=0xFFFFFFFFUL; c=crcVal(c,r.magic); c=crcVal(c,r.version); c=crcVal(c,r.snapshot.count); c=crcVal(c,r.snapshot.nextIndex);
 for(size_t i=0;i<TRANSFER_HISTORY_CAPACITY;++i){ const TransferHistoryEntry& e=r.snapshot.entries[i]; c=crcVal(c,e.requestId); c=crcVal(c,e.source); c=crcVal(c,e.terminalState); c=crcVal(c,e.endReason); c=crcVal(c,e.reserved); c=crcVal(c,e.requestedLiters); c=crcVal(c,e.estimatedTransferredLiters); c=crcVal(c,e.targetLowerLevelCm); c=crcVal(c,e.finishedAtUptimeMs); }
 return c^0xFFFFFFFFUL;
}
bool isTransferHistoryRecordValid(const TransferHistoryRecord& r) { return r.magic==TRANSFER_HISTORY_STORE_MAGIC && r.version==TRANSFER_HISTORY_STORE_VERSION && isTransferHistorySnapshotValid(r.snapshot) && r.checksum==transferHistoryRecordChecksum(r); }
NvsTransferHistoryStore::NvsTransferHistoryStore():opened_(false){}
bool NvsTransferHistoryStore::begin(){ if(opened_)return true; opened_=preferences_.begin("hwc_xfer",false); return opened_; }
void NvsTransferHistoryStore::end(){ if(opened_)preferences_.end(); opened_=false; }
bool NvsTransferHistoryStore::hasRecord(){ return opened_ && preferences_.isKey("history"); }
bool NvsTransferHistoryStore::load(TransferHistorySnapshot& out){ if(!opened_||!preferences_.isKey("history")||preferences_.getBytesLength("history")!=sizeof(TransferHistoryRecord))return false; TransferHistoryRecord r={}; if(preferences_.getBytes("history",&r,sizeof(r))!=sizeof(r)||!isTransferHistoryRecordValid(r))return false; out=r.snapshot; return true; }
bool NvsTransferHistoryStore::save(const TransferHistorySnapshot& s){ if(!opened_||!isTransferHistorySnapshotValid(s))return false; if(preferences_.isKey("history")){ TransferHistorySnapshot old={}; if(!load(old))return false; } TransferHistoryRecord r={}; r.magic=TRANSFER_HISTORY_STORE_MAGIC; r.version=TRANSFER_HISTORY_STORE_VERSION; r.snapshot=s; r.checksum=transferHistoryRecordChecksum(r); if(preferences_.putBytes("history",&r,sizeof(r))!=sizeof(r))return false; TransferHistorySnapshot verify={}; return load(verify)&&sameSnapshot(verify,s); }
}
