#ifndef HWC_NVS_LOWER_TANK_PUMP_PROTECTION_STORE_H
#define HWC_NVS_LOWER_TANK_PUMP_PROTECTION_STORE_H
#include <Preferences.h>
#include "LowerTankPumpProtection.h"
namespace HWC {
class NvsLowerTankPumpProtectionStore {
public:
    explicit NvsLowerTankPumpProtectionStore(const char* nameSpace = "hwc_lowpump");
    bool begin(); void end(); bool hasRecord();
    bool load(LowerTankPumpConfig& config, bool& criticalLatch, bool& stateValid);
    bool save(const LowerTankPumpConfig& config, bool criticalLatch, bool stateValid = true);
    bool saveCriticalLatch(bool criticalLatch);
private:
    Preferences preferences_; const char* nameSpace_; bool opened_;
};
}
#endif
