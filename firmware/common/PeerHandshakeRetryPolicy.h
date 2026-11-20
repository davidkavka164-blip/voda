#ifndef HWC_PEER_HANDSHAKE_RETRY_POLICY_H
#define HWC_PEER_HANDSHAKE_RETRY_POLICY_H

#include <stdint.h>

namespace HWC {

// Bounded retransmission policy for one immutable handshake datagram.
// The caller sends the first datagram immediately, then calls markSent().
// millis()-style arithmetic is wrap-safe for intervals <= INT32_MAX.
class PeerHandshakeRetryPolicy {
public:
    PeerHandshakeRetryPolicy();
    bool start(uint32_t nowMs, uint32_t retryIntervalMs, uint8_t maxAttempts);
    bool due(uint32_t nowMs) const;
    bool markSent(uint32_t nowMs);
    bool exhausted(uint32_t nowMs) const;
    uint8_t attempts() const;
    bool active() const;
    void cancel();

private:
    uint32_t lastSentAtMs_;
    uint32_t retryIntervalMs_;
    uint8_t maxAttempts_;
    uint8_t attempts_;
    bool active_;
};

} // namespace HWC
#endif
