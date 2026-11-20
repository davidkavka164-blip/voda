#ifndef HWC_NVS_WELL_PROTECTION_STORE_H
#define HWC_NVS_WELL_PROTECTION_STORE_H

#include <Preferences.h>
#include "WellProtectionConfig.h"

namespace HWC {

static const uint32_t WELL_PROTECTION_RECORD_MAGIC = 0x48575052UL; // HWPR
static const uint16_t WELL_PROTECTION_RECORD_VERSION = 1;

struct WellProtectionRecord {
    uint32_t magic;
    uint16_t version;
    WellProtectionConfig config;
    uint8_t criticalLatch;
    uint8_t safetyStateValid;
    uint32_t checksum;
};

uint32_t wellProtectionRecordChecksum(const WellProtectionRecord& record);
bool isWellProtectionRecordValid(const WellProtectionRecord& record);

class NvsWellProtectionStore {
public:
    explicit NvsWellProtectionStore(const char* nameSpace = "hwc_protect");
    bool begin();
    void end();
    bool hasRecord();
    bool load(WellProtectionConfig& config, bool& criticalLatch, bool& safetyStateValid);
    bool save(const WellProtectionConfig& config, bool criticalLatch, bool safetyStateValid);
    bool saveCriticalLatch(bool criticalLatch);
private:
    Preferences preferences_;
    const char* nameSpace_;
    bool opened_;
};

} // namespace HWC
#endif
