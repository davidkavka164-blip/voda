#ifndef HWC_SAMPLE_FRESHNESS_H
#define HWC_SAMPLE_FRESHNESS_H

#include <stdint.h>

namespace HWC {

// Wrap-safe freshness check for independent sensor timestamps.
inline bool sampleIsFresh(bool sampleAvailable, uint32_t nowMs,
                          uint32_t sampledAtMs, uint32_t maxAgeMs) {
    return sampleAvailable &&
           static_cast<uint32_t>(nowMs - sampledAtMs) <= maxAgeMs;
}

} // namespace HWC

#endif
