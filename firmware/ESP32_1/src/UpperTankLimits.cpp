#include "UpperTankLimits.h"
#include <math.h>
namespace HWC {
namespace {
uint32_t crcByte(uint32_t crc, uint8_t byte) { crc ^= byte; for (uint8_t i=0;i<8;++i) { const uint32_t mask=static_cast<uint32_t>(-(static_cast<int32_t>(crc&1U))); crc=(crc>>1)^(0xEDB88320UL&mask); } return crc; }
template <typename T> uint32_t crcValue(uint32_t crc,const T& value) { const uint8_t* p=reinterpret_cast<const uint8_t*>(&value); for(size_t i=0;i<sizeof(T);++i) crc=crcByte(crc,p[i]); return crc; }
}
UpperTankLimits emptyUpperTankLimits() { UpperTankLimits c={}; c.magic=UPPER_TANK_LIMITS_MAGIC; c.version=UPPER_TANK_LIMITS_VERSION; updateUpperTankLimitsChecksum(c); return c; }
bool isUpperTankLimitsValid(const UpperTankLimits& c) {
 return c.magic==UPPER_TANK_LIMITS_MAGIC && c.version==UPPER_TANK_LIMITS_VERSION &&
  isfinite(c.usableDepthCm) && c.usableDepthCm>=20.0f && c.usableDepthCm<=2000.0f &&
  isfinite(c.pumpCriticalTripCm) && c.pumpCriticalTripCm>=0.0f && c.pumpCriticalTripCm<c.usableDepthCm &&
  isfinite(c.pumpRecoveryCm) && c.pumpRecoveryCm>c.pumpCriticalTripCm && c.pumpRecoveryCm<=c.usableDepthCm &&
  isfinite(c.householdReserveCm) && c.householdReserveCm>c.pumpCriticalTripCm && c.householdReserveCm<c.usableDepthCm &&
  isfinite(c.transferValveOpenCm) && c.transferValveOpenCm>c.householdReserveCm && c.transferValveOpenCm<=c.usableDepthCm &&
  upperTankLimitsChecksumMatches(c);
}
uint32_t upperTankLimitsChecksum(const UpperTankLimits& c) { uint32_t crc=0xFFFFFFFFUL; crc=crcValue(crc,c.magic); crc=crcValue(crc,c.version); crc=crcValue(crc,c.usableDepthCm); crc=crcValue(crc,c.pumpCriticalTripCm); crc=crcValue(crc,c.pumpRecoveryCm); crc=crcValue(crc,c.householdReserveCm); crc=crcValue(crc,c.transferValveOpenCm); return crc^0xFFFFFFFFUL; }
bool upperTankLimitsChecksumMatches(const UpperTankLimits& c) { return c.checksum==upperTankLimitsChecksum(c); }
void updateUpperTankLimitsChecksum(UpperTankLimits& c) { c.checksum=upperTankLimitsChecksum(c); }
UpperTankValveDecision evaluateUpperTankTransferValve(const UpperTankLimits& c,float level,bool sensorValid,bool previousOpen) {
 UpperTankValveDecision d={false,false,false};
 if(!isUpperTankLimitsValid(c)||!sensorValid||!isfinite(level)||level<0.0f||level>c.usableDepthCm) return d;
 d.valid=true;
 d.latchedOpen=previousOpen ? level>c.householdReserveCm : level>=c.transferValveOpenCm;
 d.permit=d.latchedOpen && level>c.householdReserveCm;
 return d;
}
}
