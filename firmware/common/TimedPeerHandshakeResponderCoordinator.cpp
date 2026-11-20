#include "TimedPeerHandshakeResponderCoordinator.h"
#include <string.h>

namespace HWC {

TimedPeerHandshakeResponderCoordinator::TimedPeerHandshakeResponderCoordinator()
    : retryIntervalMs_(0U), maxAttempts_(0U), state_(PEER_HS_TIMED_RESP_IDLE) {
    memset(&challenge_, 0, sizeof(challenge_));
    memset(&ready_, 0, sizeof(ready_));
}

TimedPeerHandshakeResponderCoordinator::~TimedPeerHandshakeResponderCoordinator() { cancel(); }

bool TimedPeerHandshakeResponderCoordinator::begin(
        uint32_t nowMs, uint32_t timeoutMs, uint32_t retryIntervalMs,
        uint8_t maxAttempts, uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
        uint32_t localBootId, const PeerHandshakeHello& hello,
        const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
        const uint8_t* secret, size_t secretLength,
        IPeerReplayStore& replayStore, PeerHandshakeChallenge& challenge) {
    cancel();
    memset(&challenge, 0, sizeof(challenge));
    if (retryIntervalMs == 0U || maxAttempts == 0U ||
        !coordinator_.begin(nowMs, timeoutMs, expectedPeerDeviceId, localDeviceId,
                            localBootId, hello, responderNonce, secret, secretLength,
                            replayStore, challenge_) ||
        !challengeRetry_.start(nowMs, retryIntervalMs, maxAttempts)) {
        cancel();
        state_ = PEER_HS_TIMED_RESP_FAILED;
        return false;
    }
    retryIntervalMs_ = retryIntervalMs;
    maxAttempts_ = maxAttempts;
    challenge = challenge_;
    state_ = PEER_HS_TIMED_RESP_CHALLENGE_PENDING;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::initialChallenge(PeerHandshakeChallenge& out) const {
    memset(&out, 0, sizeof(out));
    if (state_ != PEER_HS_TIMED_RESP_CHALLENGE_PENDING) return false;
    out = challenge_;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::challengeRetryDue(uint32_t nowMs) const {
    return state_ == PEER_HS_TIMED_RESP_CHALLENGE_PENDING && challengeRetry_.due(nowMs);
}

bool TimedPeerHandshakeResponderCoordinator::challengeForRetry(
        uint32_t nowMs, PeerHandshakeChallenge& out) {
    memset(&out, 0, sizeof(out));
    if (!challengeRetryDue(nowMs)) return false;
    out = challenge_;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::markChallengeSent(uint32_t nowMs) {
    return markChallengeAttempted(nowMs);
}

bool TimedPeerHandshakeResponderCoordinator::markChallengeAttempted(uint32_t nowMs) {
    return challengeRetryDue(nowMs) && challengeRetry_.markSent(nowMs);
}

bool TimedPeerHandshakeResponderCoordinator::acceptFinish(
        uint32_t nowMs, const PeerHandshakeHello& hello,
        const PeerHandshakeChallenge& challenge, const PeerHandshakeFinish& finish,
        const uint8_t* secret, size_t secretLength, PeerHandshakeReady& ready) {
    memset(&ready, 0, sizeof(ready));
    if (state_ != PEER_HS_TIMED_RESP_CHALLENGE_PENDING &&
        state_ != PEER_HS_TIMED_RESP_READY_PENDING &&
        state_ != PEER_HS_TIMED_RESP_ESTABLISHED) return false;
    const bool firstAcceptedFinish = state_ == PEER_HS_TIMED_RESP_CHALLENGE_PENDING;
    if (!coordinator_.acceptFinish(nowMs, hello, challenge, finish,
                                   secret, secretLength, ready_)) {
        if (coordinator_.state() == PEER_HS_RESP_FAILED) {
            challengeRetry_.cancel(); readyRetry_.cancel();
            state_ = PEER_HS_TIMED_RESP_FAILED;
        }
        return false;
    }
    challengeRetry_.cancel();
    // Count the first READY transmission attempt immediately. The transport
    // sends `ready` after this call; if that send fails, poll()/readyForRetry()
    // will still retry the identical cached datagram.
    if (firstAcceptedFinish &&
        !readyRetry_.start(nowMs, retryIntervalMs_, maxAttempts_)) {
        coordinator_.cancel();
        state_ = PEER_HS_TIMED_RESP_FAILED;
        return false;
    }
    ready = ready_;
    state_ = coordinator_.state() == PEER_HS_RESP_ESTABLISHED
        ? PEER_HS_TIMED_RESP_ESTABLISHED : PEER_HS_TIMED_RESP_READY_PENDING;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::initialReady(PeerHandshakeReady& out) const {
    memset(&out, 0, sizeof(out));
    if (state_ != PEER_HS_TIMED_RESP_READY_PENDING) return false;
    out = ready_;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::markReadySent(uint32_t nowMs) {
    if (state_ != PEER_HS_TIMED_RESP_READY_PENDING ||
        !coordinator_.markReadyTransmitted(nowMs)) {
        if (state_ == PEER_HS_TIMED_RESP_READY_PENDING) {
            cancel(); state_ = PEER_HS_TIMED_RESP_FAILED;
        }
        return false;
    }
    state_ = PEER_HS_TIMED_RESP_ESTABLISHED;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::readyRetryDue(uint32_t nowMs) const {
    return (state_ == PEER_HS_TIMED_RESP_READY_PENDING ||
            state_ == PEER_HS_TIMED_RESP_ESTABLISHED) && readyRetry_.due(nowMs);
}

bool TimedPeerHandshakeResponderCoordinator::readyForRetry(
        uint32_t nowMs, PeerHandshakeReady& out) {
    memset(&out, 0, sizeof(out));
    if (!readyRetryDue(nowMs)) return false;
    out = ready_;
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::markReadyRetrySent(uint32_t nowMs) {
    return markReadyRetryAttempted(nowMs, true);
}

bool TimedPeerHandshakeResponderCoordinator::markReadyRetryAttempted(
        uint32_t nowMs, bool accepted) {
    if (!readyRetryDue(nowMs) || !readyRetry_.markSent(nowMs)) return false;
    // If the initial READY send failed, the first successful retry is the
    // first transport-accepted READY and is the point at which the responder
    // may expose its session key. Failed local sends still consume an attempt
    // so a broken link cannot cause an unbounded tight retry loop.
    if (accepted && state_ == PEER_HS_TIMED_RESP_READY_PENDING) {
        if (!coordinator_.markReadyTransmitted(nowMs)) {
            cancel(); state_ = PEER_HS_TIMED_RESP_FAILED; return false;
        }
        state_ = PEER_HS_TIMED_RESP_ESTABLISHED;
    }
    return true;
}

bool TimedPeerHandshakeResponderCoordinator::poll(uint32_t nowMs) {
    if (state_ == PEER_HS_TIMED_RESP_CHALLENGE_PENDING) {
        if (challengeRetry_.exhausted(nowMs) || coordinator_.poll(nowMs)) {
            cancel(); state_ = PEER_HS_TIMED_RESP_FAILED; return true;
        }
    } else if (state_ == PEER_HS_TIMED_RESP_READY_PENDING) {
        if (readyRetry_.exhausted(nowMs) || coordinator_.poll(nowMs)) {
            cancel(); state_ = PEER_HS_TIMED_RESP_FAILED; return true;
        }
    } else if (state_ == PEER_HS_TIMED_RESP_ESTABLISHED && readyRetry_.exhausted(nowMs)) {
        readyRetry_.cancel();
    }
    return false;
}

bool TimedPeerHandshakeResponderCoordinator::copySessionKey(
        uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const {
    return coordinator_.copySessionKey(out);
}

TimedPeerHandshakeResponderState TimedPeerHandshakeResponderCoordinator::state() const { return state_; }
uint8_t TimedPeerHandshakeResponderCoordinator::challengeAttempts() const { return challengeRetry_.attempts(); }
uint8_t TimedPeerHandshakeResponderCoordinator::readyAttempts() const { return readyRetry_.attempts(); }

void TimedPeerHandshakeResponderCoordinator::cancel() {
    coordinator_.cancel(); challengeRetry_.cancel(); readyRetry_.cancel();
    memset(&challenge_, 0, sizeof(challenge_)); memset(&ready_, 0, sizeof(ready_));
    retryIntervalMs_ = 0U; maxAttempts_ = 0U;
    state_ = PEER_HS_TIMED_RESP_IDLE;
}

} // namespace HWC
