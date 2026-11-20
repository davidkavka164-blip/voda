#ifndef HWC_WEB_AUTH_RATE_LIMITER_H
#define HWC_WEB_AUTH_RATE_LIMITER_H

#include <stdint.h>
#include <stddef.h>

namespace HWC {

// Fixed-size, allocation-free per-client limiter for HTTP authentication.
// Five failed attempts within 60 s block that client for 60 s. A successful
// authentication clears only that client's entry. Millis arithmetic is
// wrap-safe for all configured intervals (< 2^31 ms).
class WebAuthRateLimiter {
public:
    static const uint8_t ENTRY_COUNT = 8U;
    static const uint8_t MAX_FAILURES = 5U;
    static const uint32_t WINDOW_MS = 60000UL;
    static const uint32_t BLOCK_MS = 60000UL;

    WebAuthRateLimiter() : nextReplacement_(0U), saturated_(false), saturatedAtMs_(0U) {
        for (uint8_t i = 0U; i < ENTRY_COUNT; ++i) clearEntry(entries_[i]);
    }

    bool isBlocked(uint32_t clientKey, uint32_t nowMs) {
        Entry* entry = find(clientKey);
        if (entry == nullptr) {
            if (saturated_ && static_cast<uint32_t>(nowMs - saturatedAtMs_) < BLOCK_MS) return true;
            saturated_ = false;
            return false;
        }
        if (!entry->blocked) {
            if (static_cast<uint32_t>(nowMs - entry->windowStartedMs) >= WINDOW_MS) {
                clearEntry(*entry);
            }
            return false;
        }
        if (static_cast<uint32_t>(nowMs - entry->blockedAtMs) >= BLOCK_MS) {
            clearEntry(*entry);
            return false;
        }
        return true;
    }

    void recordFailure(uint32_t clientKey, uint32_t nowMs) {
        Entry* entry = find(clientKey);
        if (entry == nullptr) {
            if (saturated_ && static_cast<uint32_t>(nowMs - saturatedAtMs_) < BLOCK_MS) return;
            saturated_ = false;
            entry = allocate(clientKey, nowMs);
            if (entry == nullptr) {
                saturated_ = true;
                saturatedAtMs_ = nowMs;
                return;
            }
        }
        if (entry->blocked) {
            if (static_cast<uint32_t>(nowMs - entry->blockedAtMs) < BLOCK_MS) return;
            clearEntry(*entry);
            entry->used = true;
            entry->clientKey = clientKey;
            entry->windowStartedMs = nowMs;
        }
        if (static_cast<uint32_t>(nowMs - entry->windowStartedMs) >= WINDOW_MS) {
            entry->failures = 0U;
            entry->windowStartedMs = nowMs;
        }
        if (entry->failures < MAX_FAILURES) ++entry->failures;
        if (entry->failures >= MAX_FAILURES) {
            entry->blocked = true;
            entry->blockedAtMs = nowMs;
        }
    }

    void recordSuccess(uint32_t clientKey) {
        Entry* entry = find(clientKey);
        if (entry != nullptr) clearEntry(*entry);
    }

private:
    struct Entry {
        bool used;
        bool blocked;
        uint32_t clientKey;
        uint32_t windowStartedMs;
        uint32_t blockedAtMs;
        uint8_t failures;
    };

    static void clearEntry(Entry& entry) {
        entry.used = false;
        entry.blocked = false;
        entry.clientKey = 0U;
        entry.windowStartedMs = 0U;
        entry.blockedAtMs = 0U;
        entry.failures = 0U;
    }

    Entry* find(uint32_t clientKey) {
        for (uint8_t i = 0U; i < ENTRY_COUNT; ++i) {
            if (entries_[i].used && entries_[i].clientKey == clientKey) return &entries_[i];
        }
        return nullptr;
    }

    Entry* allocate(uint32_t clientKey, uint32_t nowMs) {
        for (uint8_t i = 0U; i < ENTRY_COUNT; ++i) {
            if (entries_[i].used && entries_[i].blocked &&
                static_cast<uint32_t>(nowMs - entries_[i].blockedAtMs) >= BLOCK_MS) {
                clearEntry(entries_[i]);
            }
        }
        for (uint8_t i = 0U; i < ENTRY_COUNT; ++i) {
            if (!entries_[i].used) {
                Entry& entry = entries_[i];
                clearEntry(entry);
                entry.used = true;
                entry.clientKey = clientKey;
                entry.windowStartedMs = nowMs;
                return &entry;
            }
        }
        // Never evict an active block: that would let source churn bypass it.
        // Reuse only a non-blocked slot. If all slots are blocked, fail closed
        // for unknown clients for one block interval rather than dropping state.
        for (uint8_t n = 0U; n < ENTRY_COUNT; ++n) {
            const uint8_t i = static_cast<uint8_t>((nextReplacement_ + n) % ENTRY_COUNT);
            if (!entries_[i].blocked) {
                Entry& entry = entries_[i];
                clearEntry(entry);
                entry.used = true;
                entry.clientKey = clientKey;
                entry.windowStartedMs = nowMs;
                nextReplacement_ = static_cast<uint8_t>((i + 1U) % ENTRY_COUNT);
                return &entry;
            }
        }
        return nullptr;
    }

    Entry entries_[ENTRY_COUNT];
    uint8_t nextReplacement_;
    bool saturated_;
    uint32_t saturatedAtMs_;
};

} // namespace HWC

#endif
