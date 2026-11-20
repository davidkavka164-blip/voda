#ifndef HWC_NVS_LOWER_TANK_INFLOW_WATCHDOG_STORE_H
#define HWC_NVS_LOWER_TANK_INFLOW_WATCHDOG_STORE_H
#include <Preferences.h>
#include "LowerTankInflowWatchdog.h"
namespace HWC {
struct LowerTankInflowWatchdogSettings {
    uint32_t startupGraceSeconds;
    uint32_t noProgressTimeoutSeconds;
};
static const uint32_t LOWER_INFLOW_WATCHDOG_MIN_SECONDS = 1U;
static const uint32_t LOWER_INFLOW_WATCHDOG_MAX_SECONDS = 3600U;
LowerTankInflowWatchdogSettings defaultLowerTankInflowWatchdogSettings();
bool isLowerTankInflowWatchdogSettingsValid(const LowerTankInflowWatchdogSettings& settings);
class NvsLowerTankInflowWatchdogStore {
public:
    explicit NvsLowerTankInflowWatchdogStore(const char* nameSpace = "hwc_inflowwd");
    bool begin();
    void end();
    bool hasRecord();
    bool load(LowerTankInflowWatchdogSettings& settings);
    bool save(const LowerTankInflowWatchdogSettings& settings);
private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;
};
}
#endif
