#include "PeerHandshakeTimeout.h"
#include <limits.h>

namespace HWC {

PeerHandshakeTimeout::PeerHandshakeTimeout()
    : startedAtMs_(0U), timeoutMs_(0U), active_(false) {}

bool PeerHandshakeTimeout::start(uint32_t nowMs, uint32_t timeoutMs) {
    cancel();
    if (timeoutMs == 0U || timeoutMs > static_cast<uint32_t>(INT32_MAX)) return false;
    startedAtMs_ = nowMs;
    timeoutMs_ = timeoutMs;
    active_ = true;
    return true;
}

bool PeerHandshakeTimeout::expired(uint32_t nowMs) const {
    return active_ && static_cast<uint32_t>(nowMs - startedAtMs_) >= timeoutMs_;
}

bool PeerHandshakeTimeout::active() const { return active_; }

bool PeerHandshakeTimeout::consume(uint32_t nowMs) {
    if (!active_ || expired(nowMs)) {
        cancel();
        return false;
    }
    cancel();
    return true;
}

void PeerHandshakeTimeout::cancel() {
    startedAtMs_ = 0U;
    timeoutMs_ = 0U;
    active_ = false;
}

} // namespace HWC
