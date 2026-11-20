#ifndef HWC_UPPER_TANK_LIMITS_H
#define HWC_UPPER_TANK_LIMITS_H
#include <stdint.h>
namespace HWC {
static const uint32_t UPPER_TANK_LIMITS_MAGIC = 0x4855544CUL; // HUTL
static const uint16_t UPPER_TANK_LIMITS_VERSION = 1;
struct UpperTankLimits {
    uint32_t magic;
    uint16_t version;
    float usableDepthCm;
    float pumpCriticalTripCm;
    float pumpRecoveryCm;
    float householdReserveCm;
    float transferValveOpenCm;
    uint32_t checksum;
};
struct UpperTankValveDecision { bool permit; bool latchedOpen; bool valid; };
UpperTankLimits emptyUpperTankLimits();
bool isUpperTankLimitsValid(const UpperTankLimits& limits);
uint32_t upperTankLimitsChecksum(const UpperTankLimits& limits);
bool upperTankLimitsChecksumMatches(const UpperTankLimits& limits);
void updateUpperTankLimitsChecksum(UpperTankLimits& limits);
UpperTankValveDecision evaluateUpperTankTransferValve(const UpperTankLimits& limits, float levelCm, bool sensorValid, bool previousOpen);
}
#endif
