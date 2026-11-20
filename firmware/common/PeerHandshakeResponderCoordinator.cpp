#include "PeerHandshakeResponderCoordinator.h"
#include <string.h>

namespace HWC {

PeerHandshakeResponderCoordinator::PeerHandshakeResponderCoordinator()
    : state_(PEER_HS_RESP_IDLE) {
    memset(&ready_, 0, sizeof(ready_));
    memset(&acceptedFinish_, 0, sizeof(acceptedFinish_));
    memset(sessionKey_, 0, sizeof(sessionKey_));
}

PeerHandshakeResponderCoordinator::~PeerHandshakeResponderCoordinator() { cancel(); }

bool PeerHandshakeResponderCoordinator::begin(
        uint32_t nowMs, uint32_t timeoutMs,
        uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
        uint32_t localBootId, const PeerHandshakeHello& hello,
        const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
        const uint8_t* secret, size_t secretLength,
        IPeerReplayStore& replayStore, PeerHandshakeChallenge& challenge) {
    cancel();
    memset(&challenge, 0, sizeof(challenge));
    if (!timeout_.start(nowMs, timeoutMs)) return false;
    if (!responder_.beginWithReplayStore(expectedPeerDeviceId, localDeviceId,
            localBootId, hello, responderNonce, secret, secretLength,
            replayStore, challenge)) {
        cancel();
        state_ = PEER_HS_RESP_FAILED;
        return false;
    }
    state_ = PEER_HS_RESP_WAIT_FINISH;
    return true;
}

bool PeerHandshakeResponderCoordinator::acceptFinish(
        uint32_t nowMs, const PeerHandshakeHello& hello,
        const PeerHandshakeChallenge& challenge,
        const PeerHandshakeFinish& finish, const uint8_t* secret,
        size_t secretLength, PeerHandshakeReady& ready) {
    memset(&ready, 0, sizeof(ready));
    // FINISH is retransmitted when READY was lost. Once accepted, only the
    // exact same proof may retrieve the cached READY; never run admission or
    // derive a second session from a different datagram.
    if (state_ == PEER_HS_RESP_READY_PENDING && timeout_.expired(nowMs)) {
        cancel(); state_ = PEER_HS_RESP_FAILED; return false;
    }
    if (state_ == PEER_HS_RESP_READY_PENDING || state_ == PEER_HS_RESP_ESTABLISHED) {
        if (memcmp(&finish, &acceptedFinish_, sizeof(acceptedFinish_)) != 0)
            return false;
        ready = ready_;
        return true;
    }
    if (state_ != PEER_HS_RESP_WAIT_FINISH || timeout_.expired(nowMs)) {
        cancel(); state_ = PEER_HS_RESP_FAILED; return false;
    }
    uint8_t candidateKey[PEER_HANDSHAKE_TAG_SIZE] = {0};
    if (!responder_.acceptFinish(hello, challenge, finish, secret,
                                 secretLength, candidateKey) ||
        !createPeerHandshakeReady(candidateKey, sizeof(candidateKey), ready_)) {
        memset(candidateKey, 0, sizeof(candidateKey));
        cancel(); state_ = PEER_HS_RESP_FAILED; return false;
    }
    memcpy(sessionKey_, candidateKey, sizeof(sessionKey_));
    acceptedFinish_ = finish;
    memset(candidateKey, 0, sizeof(candidateKey));
    ready = ready_;
    state_ = PEER_HS_RESP_READY_PENDING;
    return true;
}

bool PeerHandshakeResponderCoordinator::readyForRetry(PeerHandshakeReady& ready) const {
    memset(&ready, 0, sizeof(ready));
    if (state_ != PEER_HS_RESP_READY_PENDING && state_ != PEER_HS_RESP_ESTABLISHED)
        return false;
    ready = ready_;
    return true;
}

bool PeerHandshakeResponderCoordinator::markReadyTransmitted(uint32_t nowMs) {
    if (state_ != PEER_HS_RESP_READY_PENDING || timeout_.expired(nowMs)) {
        cancel(); state_ = PEER_HS_RESP_FAILED; return false;
    }
    timeout_.cancel();
    state_ = PEER_HS_RESP_ESTABLISHED;
    return true;
}

bool PeerHandshakeResponderCoordinator::poll(uint32_t nowMs) {
    if ((state_ == PEER_HS_RESP_WAIT_FINISH || state_ == PEER_HS_RESP_READY_PENDING) &&
        timeout_.expired(nowMs)) {
        cancel(); state_ = PEER_HS_RESP_FAILED; return true;
    }
    return false;
}

bool PeerHandshakeResponderCoordinator::copySessionKey(
        uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const {
    if (!out) return false;
    memset(out, 0, PEER_HANDSHAKE_TAG_SIZE);
    if (state_ != PEER_HS_RESP_ESTABLISHED) return false;
    memcpy(out, sessionKey_, sizeof(sessionKey_));
    return true;
}

PeerHandshakeResponderState PeerHandshakeResponderCoordinator::state() const { return state_; }

void PeerHandshakeResponderCoordinator::cancel() {
    responder_.cancel(); timeout_.cancel();
    memset(&ready_, 0, sizeof(ready_));
    memset(&acceptedFinish_, 0, sizeof(acceptedFinish_));
    memset(sessionKey_, 0, sizeof(sessionKey_));
    state_ = PEER_HS_RESP_IDLE;
}

} // namespace HWC
