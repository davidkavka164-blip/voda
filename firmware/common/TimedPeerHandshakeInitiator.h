#ifndef HWC_TIMED_PEER_HANDSHAKE_INITIATOR_H
#define HWC_TIMED_PEER_HANDSHAKE_INITIATOR_H

#include "PeerHandshakeInitiator.h"
#include "PeerHandshakeRetryPolicy.h"
#include "PeerHandshakeTimeout.h"

namespace HWC {

enum TimedPeerHandshakeInitiatorState : uint8_t {
    PEER_HS_TIMED_INIT_IDLE = 0U,
    PEER_HS_TIMED_INIT_HELLO_PENDING = 1U,
    PEER_HS_TIMED_INIT_FINISH_PENDING = 2U,
    PEER_HS_TIMED_INIT_ESTABLISHED = 3U,
    PEER_HS_TIMED_INIT_FAILED = 4U
};

// Bounded, wrap-safe initiator lifecycle. The caller sends the first HELLO
// immediately after begin(), then asks for exact cached retries when due.
// READY must be authenticated before the coordinator reports ESTABLISHED.
class TimedPeerHandshakeInitiator {
public:
    TimedPeerHandshakeInitiator();
    ~TimedPeerHandshakeInitiator();
    TimedPeerHandshakeInitiator(const TimedPeerHandshakeInitiator&) = delete;
    TimedPeerHandshakeInitiator& operator=(const TimedPeerHandshakeInitiator&) = delete;

    bool begin(uint32_t nowMs, uint32_t timeoutMs, uint32_t retryIntervalMs,
               uint8_t maxAttempts, const PeerHandshakeHello& hello,
               const uint8_t* secret, size_t secretLength,
               PeerSequenceAllocator& sequenceAllocator);
    bool initialHello(PeerHandshakeHello& out) const;
    bool helloRetryDue(uint32_t nowMs) const;
    bool helloForRetry(uint32_t nowMs, PeerHandshakeHello& out);
    bool markHelloSent(uint32_t nowMs);
    bool acceptChallenge(uint32_t nowMs, const PeerHandshakeChallenge& challenge,
                         PeerHandshakeFinish& finish);
    bool finishRetryDue(uint32_t nowMs) const;
    bool finishForRetry(uint32_t nowMs, PeerHandshakeFinish& out);
    bool markFinishSent(uint32_t nowMs);
    bool acceptReady(uint32_t nowMs, const PeerHandshakeReady& ready);
    bool poll(uint32_t nowMs);
    bool copySessionKey(uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const;
    TimedPeerHandshakeInitiatorState state() const;
    uint8_t attempts() const;
    void cancel();

private:
    PeerHandshakeInitiator initiator_;
    PeerHandshakeRetryPolicy retry_;
    PeerHandshakeTimeout timeout_;
    TimedPeerHandshakeInitiatorState state_;
    uint32_t retryIntervalMs_;
    uint8_t maxAttempts_;
};

} // namespace HWC
#endif
