#include "PeerHandshakeRetryPolicy.h"
#include <limits.h>

namespace HWC {

PeerHandshakeRetryPolicy::PeerHandshakeRetryPolicy()
    : lastSentAtMs_(0U), retryIntervalMs_(0U), maxAttempts_(0U),
      attempts_(0U), active_(false) {}

bool PeerHandshakeRetryPolicy::start(uint32_t nowMs, uint32_t retryIntervalMs,
                                     uint8_t maxAttempts) {
    cancel();
    if (retryIntervalMs == 0U || retryIntervalMs > static_cast<uint32_t>(INT32_MAX) ||
        maxAttempts == 0U) return false;
    lastSentAtMs_ = nowMs;
    retryIntervalMs_ = retryIntervalMs;
    maxAttempts_ = maxAttempts;
    attempts_ = 1U; // first datagram is sent immediately by the caller
    active_ = true;
    return true;
}

bool PeerHandshakeRetryPolicy::due(uint32_t nowMs) const {
    return active_ && attempts_ < maxAttempts_ &&
           static_cast<uint32_t>(nowMs - lastSentAtMs_) >= retryIntervalMs_;
}

bool PeerHandshakeRetryPolicy::markSent(uint32_t nowMs) {
    if (!due(nowMs)) return false;
    lastSentAtMs_ = nowMs;
    ++attempts_;
    return true;
}

bool PeerHandshakeRetryPolicy::exhausted(uint32_t nowMs) const {
    if (!active_) return false;
    return attempts_ >= maxAttempts_ &&
           static_cast<uint32_t>(nowMs - lastSentAtMs_) >= retryIntervalMs_;
}

uint8_t PeerHandshakeRetryPolicy::attempts() const { return attempts_; }
bool PeerHandshakeRetryPolicy::active() const { return active_; }

void PeerHandshakeRetryPolicy::cancel() {
    lastSentAtMs_ = 0U;
    retryIntervalMs_ = 0U;
    maxAttempts_ = 0U;
    attempts_ = 0U;
    active_ = false;
}

} // namespace HWC
