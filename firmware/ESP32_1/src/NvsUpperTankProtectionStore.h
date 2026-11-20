#ifndef HWC_NVS_UPPER_TANK_PROTECTION_STORE_H
#define HWC_NVS_UPPER_TANK_PROTECTION_STORE_H
#include <Preferences.h>
namespace HWC {
struct UpperTankSafetyState { bool criticalLatch; bool stateValid; };
class NvsUpperTankProtectionStore {
public:
    explicit NvsUpperTankProtectionStore(const char* nameSpace = "hwc_up_safety");
    bool begin();
    void end();
    bool hasRecord();
    bool load(UpperTankSafetyState& state);
    bool save(bool criticalLatch, bool stateValid = true);
private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;
};
}
#endif
