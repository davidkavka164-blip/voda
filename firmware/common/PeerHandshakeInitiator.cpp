#include "PeerHandshakeInitiator.h"
#include <string.h>

namespace HWC {

PeerHandshakeInitiator::PeerHandshakeInitiator()
    : secretLength_(0U), replayFloor_(0U), sequenceAllocator_(0),
      state_(PEER_HS_INIT_IDLE) {
    memset(&hello_, 0, sizeof(hello_));
    memset(&finish_, 0, sizeof(finish_));
    memset(secret_, 0, sizeof(secret_));
    memset(sessionKey_, 0, sizeof(sessionKey_));
}

PeerHandshakeInitiator::~PeerHandshakeInitiator() { cancel(); }

bool PeerHandshakeInitiator::begin(const PeerHandshakeHello& hello,
                                   const uint8_t* secret, size_t secretLength,
                                   PeerSequenceAllocator& sequenceAllocator) {
    cancel();
    if (hello.initiatorDeviceId == 0U || hello.responderDeviceId == 0U ||
        hello.initiatorDeviceId == hello.responderDeviceId ||
        hello.initiatorBootId == 0U || secret == 0 ||
        secretLength < PEER_HANDSHAKE_MIN_SECRET_SIZE || secretLength > sizeof(secret_) ||
        sequenceAllocator.localBootId() != hello.initiatorBootId ||
        sequenceAllocator.exhausted()) {
        state_ = PEER_HS_INIT_FAILED;
        return false;
    }
    uint8_t nonceAggregate = 0U;
    for (size_t i = 0U; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        nonceAggregate |= hello.initiatorNonce[i];
    if (nonceAggregate == 0U) {
        state_ = PEER_HS_INIT_FAILED;
        return false;
    }
    hello_ = hello;
    memcpy(secret_, secret, secretLength);
    secretLength_ = secretLength;
    sequenceAllocator_ = &sequenceAllocator;
    state_ = PEER_HS_INIT_WAIT_CHALLENGE;
    return true;
}

bool PeerHandshakeInitiator::hello(PeerHandshakeHello& out) const {
    memset(&out, 0, sizeof(out));
    if (state_ != PEER_HS_INIT_WAIT_CHALLENGE) return false;
    out = hello_;
    return true;
}

bool PeerHandshakeInitiator::acceptChallenge(
        const PeerHandshakeChallenge& challenge, PeerHandshakeFinish& finish) {
    memset(&finish, 0, sizeof(finish));
    if (state_ != PEER_HS_INIT_WAIT_CHALLENGE || sequenceAllocator_ == 0) return false;
    uint8_t candidateKey[PEER_HANDSHAKE_TAG_SIZE];
    PeerHandshakeFinish candidateFinish;
    memset(candidateKey, 0, sizeof(candidateKey));
    memset(&candidateFinish, 0, sizeof(candidateFinish));
    if (!verifyPeerHandshakeChallenge(hello_, challenge, secret_, secretLength_,
                                      candidateFinish, candidateKey)) {
        memset(candidateKey, 0, sizeof(candidateKey));
        memset(&candidateFinish, 0, sizeof(candidateFinish));
        memset(secret_, 0, sizeof(secret_));
        secretLength_ = 0U;
        state_ = PEER_HS_INIT_FAILED;
        return false;
    }
    // The floor is applied only after challenge HMAC and transcript binding
    // have succeeded. A floor exhaustion fails closed before FINISH is sent.
    if (!sequenceAllocator_->advancePastAuthenticatedFloor(
            challenge.authenticatedReplayFloor)) {
        memset(candidateKey, 0, sizeof(candidateKey));
        memset(&candidateFinish, 0, sizeof(candidateFinish));
        memset(secret_, 0, sizeof(secret_));
        secretLength_ = 0U;
        state_ = PEER_HS_INIT_FAILED;
        return false;
    }
    finish_ = candidateFinish;
    memcpy(sessionKey_, candidateKey, sizeof(sessionKey_));
    replayFloor_ = challenge.authenticatedReplayFloor;
    memset(candidateKey, 0, sizeof(candidateKey));
    memset(&candidateFinish, 0, sizeof(candidateFinish));
    finish = finish_;
    state_ = PEER_HS_INIT_WAIT_READY;
    return true;
}

bool PeerHandshakeInitiator::finishForRetry(PeerHandshakeFinish& out) const {
    memset(&out, 0, sizeof(out));
    if (state_ != PEER_HS_INIT_WAIT_READY) return false;
    out = finish_;
    return true;
}

bool PeerHandshakeInitiator::acceptReady(const PeerHandshakeReady& ready) {
    if (state_ != PEER_HS_INIT_WAIT_READY ||
        !verifyPeerHandshakeReady(sessionKey_, sizeof(sessionKey_), ready)) {
        state_ = PEER_HS_INIT_FAILED;
        memset(secret_, 0, sizeof(secret_));
        memset(&finish_, 0, sizeof(finish_));
        memset(sessionKey_, 0, sizeof(sessionKey_));
        secretLength_ = 0U;
        sequenceAllocator_ = 0;
        return false;
    }
    memset(secret_, 0, sizeof(secret_));
    secretLength_ = 0U;
    state_ = PEER_HS_INIT_ESTABLISHED;
    return true;
}

bool PeerHandshakeInitiator::established() const {
    return state_ == PEER_HS_INIT_ESTABLISHED;
}

PeerHandshakeInitiatorState PeerHandshakeInitiator::state() const { return state_; }

bool PeerHandshakeInitiator::copySessionKey(uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const {
    if (out == 0) return false;
    memset(out, 0, PEER_HANDSHAKE_TAG_SIZE);
    if (!established()) return false;
    memcpy(out, sessionKey_, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}

uint32_t PeerHandshakeInitiator::authenticatedReplayFloor() const {
    return state_ == PEER_HS_INIT_WAIT_READY || state_ == PEER_HS_INIT_ESTABLISHED
        ? replayFloor_ : 0U;
}

void PeerHandshakeInitiator::cancel() {
    memset(&hello_, 0, sizeof(hello_));
    memset(&finish_, 0, sizeof(finish_));
    memset(secret_, 0, sizeof(secret_));
    memset(sessionKey_, 0, sizeof(sessionKey_));
    secretLength_ = 0U;
    replayFloor_ = 0U;
    sequenceAllocator_ = 0;
    state_ = PEER_HS_INIT_IDLE;
}

} // namespace HWC
