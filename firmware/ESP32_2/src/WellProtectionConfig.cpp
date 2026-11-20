#include "WellProtectionConfig.h"
#include <math.h>

namespace HWC {
namespace {
uint32_t crcByte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}
template <typename T>
uint32_t crcValue(uint32_t crc, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) crc = crcByte(crc, bytes[i]);
    return crc;
}
}

WellProtectionConfig emptyWellProtectionConfig() {
    WellProtectionConfig c = {};
    c.magic = WELL_PROTECTION_CONFIG_MAGIC;
    c.version = WELL_PROTECTION_CONFIG_VERSION;
    updateWellProtectionConfigChecksum(c);
    return c;
}

bool isWellProtectionConfigValid(const WellProtectionConfig& c, float usableDepthCm) {
    return c.magic == WELL_PROTECTION_CONFIG_MAGIC &&
           c.version == WELL_PROTECTION_CONFIG_VERSION &&
           isfinite(usableDepthCm) && usableDepthCm >= 20.0f && usableDepthCm <= 2000.0f &&
           isfinite(c.transferMinimumLevelCm) && c.transferMinimumLevelCm >= 0.0f && c.transferMinimumLevelCm <= usableDepthCm &&
           isfinite(c.criticalTripLevelCm) && c.criticalTripLevelCm >= 0.0f && c.criticalTripLevelCm < usableDepthCm &&
           isfinite(c.recoveryLevelCm) && c.recoveryLevelCm > c.criticalTripLevelCm && c.recoveryLevelCm <= usableDepthCm;
}

uint32_t wellProtectionConfigChecksum(const WellProtectionConfig& c) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcValue(crc, c.magic);
    crc = crcValue(crc, c.version);
    crc = crcValue(crc, c.transferMinimumLevelCm);
    crc = crcValue(crc, c.criticalTripLevelCm);
    crc = crcValue(crc, c.recoveryLevelCm);
    return crc ^ 0xFFFFFFFFUL;
}

bool wellProtectionConfigChecksumMatches(const WellProtectionConfig& c) {
    return c.checksum == wellProtectionConfigChecksum(c);
}

void updateWellProtectionConfigChecksum(WellProtectionConfig& c) {
    c.checksum = wellProtectionConfigChecksum(c);
}

} // namespace HWC
