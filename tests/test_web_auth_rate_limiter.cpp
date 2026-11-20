#include "WebAuthRateLimiter.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main() {
    HWC::WebAuthRateLimiter limiter;
    const uint32_t a = 0xC0A80402UL;
    const uint32_t b = 0xC0A80403UL;
    const uint32_t start = 1000UL;

    assert(!limiter.isBlocked(a, start));
    for (uint8_t i = 0U; i < 4U; ++i) limiter.recordFailure(a, start + i);
    assert(!limiter.isBlocked(a, start + 4U));
    limiter.recordFailure(a, start + 5U);
    assert(limiter.isBlocked(a, start + 5U));

    // The limiter is per client; a second client is unaffected.
    assert(!limiter.isBlocked(b, start + 5U));
    for (uint8_t i = 0U; i < 5U; ++i) limiter.recordFailure(b, start + 10U + i);
    assert(limiter.isBlocked(b, start + 14U));

    // Exact block boundary expires; the client can try again.
    assert(!limiter.isBlocked(a, start + 5U + HWC::WebAuthRateLimiter::BLOCK_MS));
    limiter.recordFailure(a, start + 5U + HWC::WebAuthRateLimiter::BLOCK_MS + 1U);
    assert(!limiter.isBlocked(a, start + 5U + HWC::WebAuthRateLimiter::BLOCK_MS + 1U));

    // Successful login clears only the matching client.
    limiter.recordSuccess(b);
    assert(!limiter.isBlocked(b, start + 20U));

    // Failure window expires independently of block state.
    HWC::WebAuthRateLimiter windowLimiter;
    for (uint8_t i = 0U; i < 4U; ++i) windowLimiter.recordFailure(a, start + i);
    windowLimiter.recordFailure(a, start + HWC::WebAuthRateLimiter::WINDOW_MS);
    assert(!windowLimiter.isBlocked(a, start + HWC::WebAuthRateLimiter::WINDOW_MS));

    // Wrap-safe expiry across UINT32_MAX.
    HWC::WebAuthRateLimiter wrapLimiter;
    const uint32_t nearWrap = UINT32_MAX - 100U;
    for (uint8_t i = 0U; i < 5U; ++i) wrapLimiter.recordFailure(a, nearWrap + i);
    assert(wrapLimiter.isBlocked(a, nearWrap + 4U));
    const uint32_t afterWrap = nearWrap + 4U + HWC::WebAuthRateLimiter::BLOCK_MS;
    assert(!wrapLimiter.isBlocked(a, afterWrap));

    // Fixed table remains bounded. Active blocks are never evicted; when all
    // entries are blocked, unknown clients fail closed during saturation.
    HWC::WebAuthRateLimiter bounded;
    for (uint32_t ip = 1U; ip <= HWC::WebAuthRateLimiter::ENTRY_COUNT; ++ip) {
        const uint32_t clientStart = start + ip * 10U;
        for (uint8_t i = 0U; i < HWC::WebAuthRateLimiter::MAX_FAILURES; ++i)
            bounded.recordFailure(ip, clientStart + i);
        assert(bounded.isBlocked(ip, start + 100U));
    }
    bounded.recordFailure(99U, start + 101U);
    assert(bounded.isBlocked(99U, start + 102U));
    assert(bounded.isBlocked(1U, start + 102U));
    assert(!bounded.isBlocked(99U, start + 101U + HWC::WebAuthRateLimiter::BLOCK_MS));

    puts("PASS: per-client HTTP auth rate limiter");
    return 0;
}
