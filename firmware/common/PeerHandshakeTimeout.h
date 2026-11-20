#ifndef HWC_PEER_HANDSHAKE_TIMEOUT_H
#define HWC_PEER_HANDSHAKE_TIMEOUT_H

#include <stdint.h>

namespace HWC {

// Wrap-safe timeout policy for one pending responder handshake.
// timeoutMs must be in [1, INT32_MAX]. The caller supplies millis()-style ticks.
class PeerHandshakeTimeout {
public:
    PeerHandshakeTimeout();
    bool start(uint32_t nowMs, uint32_t timeoutMs);
    bool expired(uint32_t nowMs) const;
    bool active() const;
    bool consume(uint32_t nowMs);
    void cancel();
private:
    uint32_t startedAtMs_;
    uint32_t timeoutMs_;
    bool active_;
};

} // namespace HWC
#endif
