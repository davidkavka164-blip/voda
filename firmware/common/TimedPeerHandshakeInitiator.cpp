#include "TimedPeerHandshakeInitiator.h"
#include <string.h>

namespace HWC {

TimedPeerHandshakeInitiator::TimedPeerHandshakeInitiator()
    : state_(PEER_HS_TIMED_INIT_IDLE), retryIntervalMs_(0U), maxAttempts_(0U) {}

TimedPeerHandshakeInitiator::~TimedPeerHandshakeInitiator() { cancel(); }

bool TimedPeerHandshakeInitiator::begin(
        uint32_t nowMs, uint32_t timeoutMs, uint32_t retryIntervalMs,
        uint8_t maxAttempts, const PeerHandshakeHello& hello,
        const uint8_t* secret, size_t secretLength,
        PeerSequenceAllocator& sequenceAllocator) {
    cancel();
    if (!timeout_.start(nowMs, timeoutMs) ||
        !retry_.start(nowMs, retryIntervalMs, maxAttempts) ||
        !initiator_.begin(hello, secret, secretLength, sequenceAllocator)) {
        cancel();
        state_ = PEER_HS_TIMED_INIT_FAILED;
        return false;
    }
    retryIntervalMs_ = retryIntervalMs;
    maxAttempts_ = maxAttempts;
    state_ = PEER_HS_TIMED_INIT_HELLO_PENDING;
    return true;
}

bool TimedPeerHandshakeInitiator::initialHello(PeerHandshakeHello& out) const {
    memset(&out, 0, sizeof(out));
    return state_ == PEER_HS_TIMED_INIT_HELLO_PENDING && initiator_.hello(out);
}

bool TimedPeerHandshakeInitiator::helloRetryDue(uint32_t nowMs) const {
    return state_ == PEER_HS_TIMED_INIT_HELLO_PENDING &&
           !timeout_.expired(nowMs) && retry_.due(nowMs);
}

bool TimedPeerHandshakeInitiator::helloForRetry(
        uint32_t nowMs, PeerHandshakeHello& out) {
    memset(&out, 0, sizeof(out));
    if (!helloRetryDue(nowMs)) return false;
    return initiator_.hello(out);
}

bool TimedPeerHandshakeInitiator::markHelloSent(uint32_t nowMs) {
    if (state_ != PEER_HS_TIMED_INIT_HELLO_PENDING || timeout_.expired(nowMs) ||
        !retry_.markSent(nowMs)) return false;
    return true;
}

bool TimedPeerHandshakeInitiator::acceptChallenge(
        uint32_t nowMs, const PeerHandshakeChallenge& challenge,
        PeerHandshakeFinish& finish) {
    memset(&finish, 0, sizeof(finish));
    if (state_ != PEER_HS_TIMED_INIT_HELLO_PENDING || timeout_.expired(nowMs)) {
        poll(nowMs);
        return false;
    }
    if (!initiator_.acceptChallenge(challenge, finish)) {
        cancel(); state_ = PEER_HS_TIMED_INIT_FAILED; return false;
    }
    retry_.cancel();
    if (!retry_.start(nowMs, retryIntervalMs_, maxAttempts_)) {
        cancel(); state_ = PEER_HS_TIMED_INIT_FAILED; return false;
    }
    state_ = PEER_HS_TIMED_INIT_FINISH_PENDING;
    return true;
}

bool TimedPeerHandshakeInitiator::finishRetryDue(uint32_t nowMs) const {
    return state_ == PEER_HS_TIMED_INIT_FINISH_PENDING &&
           !timeout_.expired(nowMs) && retry_.due(nowMs);
}

bool TimedPeerHandshakeInitiator::finishForRetry(
        uint32_t nowMs, PeerHandshakeFinish& out) {
    memset(&out, 0, sizeof(out));
    if (!finishRetryDue(nowMs)) return false;
    return initiator_.finishForRetry(out);
}

bool TimedPeerHandshakeInitiator::markFinishSent(uint32_t nowMs) {
    if (state_ != PEER_HS_TIMED_INIT_FINISH_PENDING || timeout_.expired(nowMs) ||
        !retry_.markSent(nowMs)) return false;
    return true;
}

bool TimedPeerHandshakeInitiator::acceptReady(
        uint32_t nowMs, const PeerHandshakeReady& ready) {
    if (state_ != PEER_HS_TIMED_INIT_FINISH_PENDING || timeout_.expired(nowMs)) {
        poll(nowMs);
        return false;
    }
    if (!initiator_.acceptReady(ready)) {
        retry_.cancel(); timeout_.cancel();
        state_ = PEER_HS_TIMED_INIT_FAILED; return false;
    }
    retry_.cancel(); timeout_.cancel();
    state_ = PEER_HS_TIMED_INIT_ESTABLISHED;
    return true;
}

bool TimedPeerHandshakeInitiator::poll(uint32_t nowMs) {
    if ((state_ == PEER_HS_TIMED_INIT_HELLO_PENDING ||
         state_ == PEER_HS_TIMED_INIT_FINISH_PENDING) &&
        (timeout_.expired(nowMs) || retry_.exhausted(nowMs))) {
        cancel(); state_ = PEER_HS_TIMED_INIT_FAILED; return true;
    }
    return false;
}

bool TimedPeerHandshakeInitiator::copySessionKey(
        uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const {
    if (!out) return false;
    memset(out, 0, PEER_HANDSHAKE_TAG_SIZE);
    return state_ == PEER_HS_TIMED_INIT_ESTABLISHED && initiator_.copySessionKey(out);
}

TimedPeerHandshakeInitiatorState TimedPeerHandshakeInitiator::state() const { return state_; }
uint8_t TimedPeerHandshakeInitiator::attempts() const { return retry_.attempts(); }

void TimedPeerHandshakeInitiator::cancel() {
    initiator_.cancel(); retry_.cancel(); timeout_.cancel();
    state_ = PEER_HS_TIMED_INIT_IDLE;
    retryIntervalMs_ = 0U; maxAttempts_ = 0U;
}

} // namespace HWC
