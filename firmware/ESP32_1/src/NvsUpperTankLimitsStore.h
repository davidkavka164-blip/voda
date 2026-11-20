#ifndef HWC_NVS_UPPER_TANK_LIMITS_STORE_H
#define HWC_NVS_UPPER_TANK_LIMITS_STORE_H
#include <Preferences.h>
#include "UpperTankLimits.h"
namespace HWC {
class NvsUpperTankLimitsStore {
public:
 explicit NvsUpperTankLimitsStore(const char* nameSpace="hwc_upperlimits");
 bool begin(); void end(); bool hasRecord(); bool load(UpperTankLimits& limits); bool save(const UpperTankLimits& limits);
private: Preferences preferences_; const char* nameSpace_; bool opened_;
};
}
#endif
