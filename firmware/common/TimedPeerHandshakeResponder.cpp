#include "TimedPeerHandshakeResponder.h"
#include <string.h>

namespace HWC {

bool TimedPeerHandshakeResponder::begin(
        uint32_t nowMs, uint32_t timeoutMs,
        uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
        uint32_t localBootId, const PeerHandshakeHello& hello,
        const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
        const uint8_t* secret, size_t secretLength,
        PeerHandshakeChallenge& challenge) {
    cancel();
    if (!timeout_.start(nowMs, timeoutMs)) return false;
    if (!responder_.begin(expectedPeerDeviceId, localDeviceId, localBootId,
                          hello, responderNonce, secret, secretLength, challenge)) {
        cancel();
        return false;
    }
    return true;
}

bool TimedPeerHandshakeResponder::acceptFinish(
        uint32_t nowMs, const PeerHandshakeHello& hello,
        const PeerHandshakeChallenge& challenge,
        const PeerHandshakeFinish& finish,
        const uint8_t* secret, size_t secretLength,
        uint8_t sessionKey[PEER_HANDSHAKE_TAG_SIZE]) {
    if (sessionKey) memset(sessionKey, 0, PEER_HANDSHAKE_TAG_SIZE);
    if (!timeout_.consume(nowMs)) {
        responder_.cancel();
        return false;
    }
    return responder_.acceptFinish(hello, challenge, finish, secret, secretLength, sessionKey);
}

bool TimedPeerHandshakeResponder::poll(uint32_t nowMs) {
    if (timeout_.expired(nowMs)) {
        cancel();
        return true;
    }
    return false;
}

void TimedPeerHandshakeResponder::cancel() {
    timeout_.cancel();
    responder_.cancel();
}

bool TimedPeerHandshakeResponder::pending() const {
    return timeout_.active() && responder_.pending();
}

} // namespace HWC
