#ifndef HWC_WELL_PROTECTION_CONFIG_H
#define HWC_WELL_PROTECTION_CONFIG_H

#include <stdint.h>

namespace HWC {

static const uint32_t WELL_PROTECTION_CONFIG_MAGIC = 0x48575043UL; // HWPC
static const uint16_t WELL_PROTECTION_CONFIG_VERSION = 1;

struct WellProtectionConfig {
    uint32_t magic;
    uint16_t version;
    float transferMinimumLevelCm;
    float criticalTripLevelCm;
    float recoveryLevelCm;
    uint32_t checksum;
};

WellProtectionConfig emptyWellProtectionConfig();
bool isWellProtectionConfigValid(const WellProtectionConfig& config, float usableDepthCm);
uint32_t wellProtectionConfigChecksum(const WellProtectionConfig& config);
bool wellProtectionConfigChecksumMatches(const WellProtectionConfig& config);
void updateWellProtectionConfigChecksum(WellProtectionConfig& config);

} // namespace HWC
#endif
