#ifndef HWC_TIMED_PEER_HANDSHAKE_RESPONDER_COORDINATOR_H
#define HWC_TIMED_PEER_HANDSHAKE_RESPONDER_COORDINATOR_H

#include "PeerHandshakeResponderCoordinator.h"
#include "PeerHandshakeRetryPolicy.h"

namespace HWC {

enum TimedPeerHandshakeResponderState : uint8_t {
    PEER_HS_TIMED_RESP_IDLE = 0U,
    PEER_HS_TIMED_RESP_CHALLENGE_PENDING = 1U,
    PEER_HS_TIMED_RESP_READY_PENDING = 2U,
    PEER_HS_TIMED_RESP_ESTABLISHED = 3U,
    PEER_HS_TIMED_RESP_FAILED = 4U
};

// Bounded retransmission wrapper for the responder side. CHALLENGE and READY
// are cached and retries return the exact same structures. Retry-attempt
// methods must be called after each actual send attempt (including local UDP
// failures) so a broken link cannot create a tight, unbounded retry loop. The
// session key remains unavailable until READY is accepted by the transport.
class TimedPeerHandshakeResponderCoordinator {
public:
    TimedPeerHandshakeResponderCoordinator();
    ~TimedPeerHandshakeResponderCoordinator();
    TimedPeerHandshakeResponderCoordinator(const TimedPeerHandshakeResponderCoordinator&) = delete;
    TimedPeerHandshakeResponderCoordinator& operator=(const TimedPeerHandshakeResponderCoordinator&) = delete;

    bool begin(uint32_t nowMs, uint32_t timeoutMs, uint32_t retryIntervalMs,
               uint8_t maxAttempts, uint8_t expectedPeerDeviceId,
               uint8_t localDeviceId, uint32_t localBootId,
               const PeerHandshakeHello& hello,
               const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
               const uint8_t* secret, size_t secretLength,
               IPeerReplayStore& replayStore, PeerHandshakeChallenge& challenge);
    bool initialChallenge(PeerHandshakeChallenge& out) const;
    bool challengeRetryDue(uint32_t nowMs) const;
    bool challengeForRetry(uint32_t nowMs, PeerHandshakeChallenge& out);
    bool markChallengeSent(uint32_t nowMs);
    // Call after every CHALLENGE transmission attempt; a local send failure
    // still consumes a bounded retry slot.
    bool markChallengeAttempted(uint32_t nowMs);
    bool acceptFinish(uint32_t nowMs, const PeerHandshakeHello& hello,
                      const PeerHandshakeChallenge& challenge,
                      const PeerHandshakeFinish& finish,
                      const uint8_t* secret, size_t secretLength,
                      PeerHandshakeReady& ready);
    bool initialReady(PeerHandshakeReady& out) const;
    bool markReadySent(uint32_t nowMs);
    bool readyRetryDue(uint32_t nowMs) const;
    bool readyForRetry(uint32_t nowMs, PeerHandshakeReady& out);
    bool markReadyRetrySent(uint32_t nowMs);
    // Call after every retry transmission attempt, even when UDP reports a
    // local send failure. `accepted` controls when the session key is released.
    bool markReadyRetryAttempted(uint32_t nowMs, bool accepted);
    bool poll(uint32_t nowMs);
    bool copySessionKey(uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const;
    TimedPeerHandshakeResponderState state() const;
    uint8_t challengeAttempts() const;
    uint8_t readyAttempts() const;
    void cancel();

private:
    PeerHandshakeResponderCoordinator coordinator_;
    PeerHandshakeRetryPolicy challengeRetry_;
    PeerHandshakeRetryPolicy readyRetry_;
    PeerHandshakeChallenge challenge_;
    PeerHandshakeReady ready_;
    uint32_t retryIntervalMs_;
    uint8_t maxAttempts_;
    TimedPeerHandshakeResponderState state_;
};

} // namespace HWC
#endif
