#include "SampleFreshness.h"
#include <assert.h>
#include <stdint.h>

int main() {
    using HWC::sampleIsFresh;
    assert(!sampleIsFresh(false, 1000U, 1000U, 5000U));
    assert(sampleIsFresh(true, 1000U, 1000U, 5000U));
    assert(sampleIsFresh(true, 6000U, 1000U, 5000U));
    assert(!sampleIsFresh(true, 6001U, 1000U, 5000U));
    // Unsigned subtraction handles millis() rollover.
    assert(sampleIsFresh(true, 3U, UINT32_MAX - 2U, 6U));
    assert(!sampleIsFresh(true, 4U, UINT32_MAX - 2U, 6U));
    // Independent availability: a missing well sample does not affect a lower sample.
    const bool lowerFresh = sampleIsFresh(true, 2500U, 2000U, 5000U);
    const bool wellFresh = sampleIsFresh(false, 2500U, 2000U, 5000U);
    assert(lowerFresh && !wellFresh);
    return 0;
}
