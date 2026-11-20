#include "NvsLowerTankInflowWatchdogStore.h"
#include <stddef.h>
namespace HWC {
namespace {
struct Record { uint32_t magic; uint16_t version; uint16_t reserved; LowerTankInflowWatchdogSettings settings; uint32_t checksum; };
uint32_t crcByte(uint32_t crc, uint8_t byte) { crc ^= byte; for (uint8_t i=0;i<8;++i) { const uint32_t mask=static_cast<uint32_t>(-(static_cast<int32_t>(crc&1U))); crc=(crc>>1)^(0xEDB88320UL&mask); } return crc; }
uint32_t checksum(const Record& r) { uint32_t c=0xFFFFFFFFUL; const uint8_t* p=reinterpret_cast<const uint8_t*>(&r); for (size_t i=0;i<offsetof(Record, checksum);++i) c=crcByte(c,p[i]); return c^0xFFFFFFFFUL; }
bool validRecord(const Record& r) { return r.magic==0x49465744UL && r.version==1U && isLowerTankInflowWatchdogSettingsValid(r.settings) && r.checksum==checksum(r); }
}
LowerTankInflowWatchdogSettings defaultLowerTankInflowWatchdogSettings() { LowerTankInflowWatchdogSettings s={30U,30U}; return s; }
bool isLowerTankInflowWatchdogSettingsValid(const LowerTankInflowWatchdogSettings& s) { return s.startupGraceSeconds>=LOWER_INFLOW_WATCHDOG_MIN_SECONDS && s.startupGraceSeconds<=LOWER_INFLOW_WATCHDOG_MAX_SECONDS && s.noProgressTimeoutSeconds>=LOWER_INFLOW_WATCHDOG_MIN_SECONDS && s.noProgressTimeoutSeconds<=LOWER_INFLOW_WATCHDOG_MAX_SECONDS; }
NvsLowerTankInflowWatchdogStore::NvsLowerTankInflowWatchdogStore(const char* n):nameSpace_(n),opened_(false){}
bool NvsLowerTankInflowWatchdogStore::begin(){if(opened_)return true;opened_=nameSpace_&&nameSpace_[0]&&preferences_.begin(nameSpace_,false);return opened_;}
void NvsLowerTankInflowWatchdogStore::end(){if(opened_)preferences_.end();opened_=false;}
bool NvsLowerTankInflowWatchdogStore::hasRecord(){return opened_&&preferences_.isKey("state");}
bool NvsLowerTankInflowWatchdogStore::load(LowerTankInflowWatchdogSettings& s){if(!opened_||!preferences_.isKey("state")||preferences_.getBytesLength("state")!=sizeof(Record))return false;Record r={};if(preferences_.getBytes("state",&r,sizeof(r))!=sizeof(r)||!validRecord(r))return false;s=r.settings;return true;}
bool NvsLowerTankInflowWatchdogStore::save(const LowerTankInflowWatchdogSettings& s){if(!opened_||!isLowerTankInflowWatchdogSettingsValid(s))return false;Record r={};r.magic=0x49465744UL;r.version=1U;r.settings=s;r.checksum=checksum(r);if(preferences_.putBytes("state",&r,sizeof(r))!=sizeof(r))return false;LowerTankInflowWatchdogSettings verify={};return load(verify)&&verify.startupGraceSeconds==s.startupGraceSeconds&&verify.noProgressTimeoutSeconds==s.noProgressTimeoutSeconds;}
}
