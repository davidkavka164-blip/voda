#include "NvsUpperTankLimitsStore.h"
namespace HWC { namespace {
const uint32_t LM=0x48555452UL, SM=0x48555453UL, HM=0x48555448UL, TM=0x48555454UL;
const uint16_t V=1; const uint8_t NONE=0xFF;
enum Mode:uint8_t { INITIAL=1, MIGRATION=2, UPDATE=3 };
struct Legacy { uint32_t magic; uint16_t version; UpperTankLimits limits; uint32_t crc; };
struct Slot { uint32_t magic; uint16_t version,payloadSize; uint32_t generation; UpperTankLimits limits; uint32_t crc; };
struct Head { uint32_t magic; uint16_t version; uint8_t slot,reserved; uint32_t generation,crc; };
struct Txn { uint32_t magic; uint16_t version; uint8_t mode,previousSlot,targetSlot,reserved[3]; uint32_t previousGeneration,targetGeneration,crc; };
uint32_t cb(uint32_t c,uint8_t b){c^=b;for(uint8_t i=0;i<8;++i){uint32_t m=static_cast<uint32_t>(-(static_cast<int32_t>(c&1U)));c=(c>>1)^(0xEDB88320UL&m);}return c;}
template<class T> uint32_t cv(uint32_t c,const T&v){const uint8_t*p=reinterpret_cast<const uint8_t*>(&v);for(size_t i=0;i<sizeof(T);++i)c=cb(c,p[i]);return c;}
uint32_t lc(uint32_t c,const UpperTankLimits&x){c=cv(c,x.magic);c=cv(c,x.version);c=cv(c,x.usableDepthCm);c=cv(c,x.pumpCriticalTripCm);c=cv(c,x.pumpRecoveryCm);c=cv(c,x.householdReserveCm);c=cv(c,x.transferValveOpenCm);return cv(c,x.checksum);}
uint32_t legacyC(const Legacy&r){uint32_t c=0xFFFFFFFFUL;c=cv(c,r.magic);c=cv(c,r.version);return lc(c,r.limits)^0xFFFFFFFFUL;}
uint32_t slotC(const Slot&r){uint32_t c=0xFFFFFFFFUL;c=cv(c,r.magic);c=cv(c,r.version);c=cv(c,r.payloadSize);c=cv(c,r.generation);return lc(c,r.limits)^0xFFFFFFFFUL;}
uint32_t headC(const Head&r){uint32_t c=0xFFFFFFFFUL;c=cv(c,r.magic);c=cv(c,r.version);c=cv(c,r.slot);c=cv(c,r.generation);return c^0xFFFFFFFFUL;}
uint32_t txnC(const Txn&r){uint32_t c=0xFFFFFFFFUL;c=cv(c,r.magic);c=cv(c,r.version);c=cv(c,r.mode);c=cv(c,r.previousSlot);c=cv(c,r.targetSlot);c=cv(c,r.previousGeneration);c=cv(c,r.targetGeneration);return c^0xFFFFFFFFUL;}
bool same(const UpperTankLimits&a,const UpperTankLimits&b){return a.magic==b.magic&&a.version==b.version&&a.usableDepthCm==b.usableDepthCm&&a.pumpCriticalTripCm==b.pumpCriticalTripCm&&a.pumpRecoveryCm==b.pumpRecoveryCm&&a.householdReserveCm==b.householdReserveCm&&a.transferValveOpenCm==b.transferValveOpenCm&&a.checksum==b.checksum;}
bool validLegacy(const Legacy&r){return r.magic==LM&&r.version==V&&isUpperTankLimitsValid(r.limits)&&r.crc==legacyC(r);}
bool validSlot(const Slot&r){return r.magic==SM&&r.version==V&&r.payloadSize==sizeof(UpperTankLimits)&&r.generation&&isUpperTankLimitsValid(r.limits)&&r.crc==slotC(r);}
bool validHead(const Head&r){return r.magic==HM&&r.version==V&&r.slot<2&&r.generation&&r.crc==headC(r);}
bool validTxn(const Txn&r){if(r.magic!=TM||r.version!=V||r.targetSlot>1||!r.targetGeneration||r.crc!=txnC(r))return false;if(r.mode==INITIAL||r.mode==MIGRATION)return r.previousSlot==NONE&&!r.previousGeneration&&r.targetSlot==0&&r.targetGeneration==1;if(r.mode==UPDATE)return r.previousSlot<2&&r.previousSlot!=r.targetSlot&&r.previousGeneration&&r.targetGeneration==r.previousGeneration+1U&&r.targetGeneration;return false;}
enum Status{ABSENT,VALID,CORRUPT};
Status read(Preferences&p,const char*k,void*out,size_t n){if(!p.isKey(k))return ABSENT;if(p.getBytesLength(k)!=n||p.getBytes(k,out,n)!=n)return CORRUPT;return VALID;}
Status slotRead(Preferences&p,uint8_t i,Slot&r){Status s=read(p,i?"slot1":"slot0",&r,sizeof(r));return s==VALID&&!validSlot(r)?CORRUPT:s;}
Status headRead(Preferences&p,Head&r){Status s=read(p,"head",&r,sizeof(r));return s==VALID&&!validHead(r)?CORRUPT:s;}
Status txnRead(Preferences&p,Txn&r){Status s=read(p,"txn",&r,sizeof(r));return s==VALID&&!validTxn(r)?CORRUPT:s;}
bool legacyRead(Preferences&p,UpperTankLimits&x){Legacy r={};if(read(p,"state",&r,sizeof(r))!=VALID||!validLegacy(r))return false;x=r.limits;return true;}
bool slotWrite(Preferences&p,uint8_t i,const UpperTankLimits&x,uint32_t g){Slot r={};r.magic=SM;r.version=V;r.payloadSize=sizeof(x);r.generation=g;r.limits=x;r.crc=slotC(r);const char*k=i?"slot1":"slot0";if(p.putBytes(k,&r,sizeof(r))!=sizeof(r))return false;Slot q={};return slotRead(p,i,q)==VALID&&q.generation==g&&same(q.limits,x);}
bool headWrite(Preferences&p,uint8_t i,uint32_t g){Head r={};r.magic=HM;r.version=V;r.slot=i;r.generation=g;r.crc=headC(r);if(p.putBytes("head",&r,sizeof(r))!=sizeof(r))return false;Head q={};return headRead(p,q)==VALID&&q.slot==i&&q.generation==g;}
Txn makeTxn(uint8_t m,uint8_t ps,uint32_t pg,uint8_t ts,uint32_t tg){Txn r={};r.magic=TM;r.version=V;r.mode=m;r.previousSlot=ps;r.targetSlot=ts;r.previousGeneration=pg;r.targetGeneration=tg;r.crc=txnC(r);return r;}
bool txnWrite(Preferences&p,const Txn&r){if(p.putBytes("txn",&r,sizeof(r))!=sizeof(r))return false;Txn q={};return txnRead(p,q)==VALID&&q.mode==r.mode&&q.previousSlot==r.previousSlot&&q.targetSlot==r.targetSlot&&q.previousGeneration==r.previousGeneration&&q.targetGeneration==r.targetGeneration;}
bool txnClear(Preferences&p){return !p.isKey("txn")||p.remove("txn");}
}
NvsUpperTankLimitsStore::NvsUpperTankLimitsStore(const char*n):nameSpace_(n),opened_(false){}
bool NvsUpperTankLimitsStore::begin(){if(opened_)return true;opened_=nameSpace_&&nameSpace_[0]&&preferences_.begin(nameSpace_,false);return opened_;}
void NvsUpperTankLimitsStore::end(){if(opened_)preferences_.end();opened_=false;}
bool NvsUpperTankLimitsStore::hasRecord(){return opened_&&(preferences_.isKey("state")||preferences_.isKey("slot0")||preferences_.isKey("slot1")||preferences_.isKey("head")||preferences_.isKey("txn"));}
bool NvsUpperTankLimitsStore::load(UpperTankLimits&x){
 if (!opened_) return false;
 Head h = {}; Txn t = {};
 Status hs = headRead(preferences_, h), ts = txnRead(preferences_, t);
 if (ts == CORRUPT) {
  if (hs != VALID) return false;
  Slot committed = {};
  if (slotRead(preferences_, h.slot, committed) != VALID || committed.generation != h.generation) return false;
  x = committed.limits; return true;
 }
 if(ts==VALID){Slot target={};Status s=slotRead(preferences_,t.targetSlot,target);if(s==VALID&&target.generation==t.targetGeneration){if(headWrite(preferences_,t.targetSlot,t.targetGeneration))txnClear(preferences_);x=target.limits;return true;}
  if(t.mode==UPDATE){Slot prev={};if(slotRead(preferences_,t.previousSlot,prev)==VALID&&prev.generation==t.previousGeneration){if(headWrite(preferences_,t.previousSlot,t.previousGeneration))txnClear(preferences_);x=prev.limits;return true;}return false;}
  if(t.mode==MIGRATION){UpperTankLimits old={};if(!legacyRead(preferences_,old)||!slotWrite(preferences_,t.targetSlot,old,t.targetGeneration))return false;if(headWrite(preferences_,t.targetSlot,t.targetGeneration))txnClear(preferences_);x=old;return true;}return false;
 }
 if (hs == CORRUPT) return false;
 if (hs == VALID) { Slot active = {}; if (slotRead(preferences_, h.slot, active) != VALID || active.generation != h.generation) return false; x = active.limits; return true; }
 Slot a={},b={};if(slotRead(preferences_,0,a)!=ABSENT||slotRead(preferences_,1,b)!=ABSENT)return false;UpperTankLimits old={};if(!legacyRead(preferences_,old))return false;
 Txn migration=makeTxn(MIGRATION,NONE,0,0,1);if(!txnWrite(preferences_,migration))return false;return load(x);
}
bool NvsUpperTankLimitsStore::save(const UpperTankLimits&x){
 if (!opened_ || !isUpperTankLimitsValid(x)) return false;
 Head h = {};
 if(hasRecord()){UpperTankLimits old={};if(!load(old)){
   Txn pending={};Head absent={};if(txnRead(preferences_,pending)!=VALID||pending.mode!=INITIAL||headRead(preferences_,absent)!=ABSENT)return false;
   if (!slotWrite(preferences_, pending.targetSlot, x, pending.targetGeneration) || !headWrite(preferences_, pending.targetSlot, pending.targetGeneration)) return false;
   txnClear(preferences_); UpperTankLimits v = {}; return load(v) && same(v, x);
  }if(headRead(preferences_,h)!=VALID)return false;
 }else if(headRead(preferences_,h)!=ABSENT)return false;
 uint8_t ps=NONE,ts=0,mode=INITIAL;uint32_t pg=0,tg=1;
 if(validHead(h)){Slot cur={};if(slotRead(preferences_,h.slot,cur)!=VALID||cur.generation!=h.generation)return false;ps=h.slot;pg=h.generation;ts=static_cast<uint8_t>(1U-h.slot);tg=pg+1U;if(!tg)return false;mode=UPDATE;}
 Txn transaction=makeTxn(mode,ps,pg,ts,tg);if(!txnWrite(preferences_,transaction))return false;if(!slotWrite(preferences_,ts,x,tg))return false;if(!headWrite(preferences_,ts,tg))return false;txnClear(preferences_);UpperTankLimits v={};return load(v)&&same(v,x);
}
}
