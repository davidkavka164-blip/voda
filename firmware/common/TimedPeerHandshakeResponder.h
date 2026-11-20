#ifndef HWC_TIMED_PEER_HANDSHAKE_RESPONDER_H
#define HWC_TIMED_PEER_HANDSHAKE_RESPONDER_H

#include "PeerHandshake.h"
#include "PeerHandshakeTimeout.h"

namespace HWC {

// Responder wrapper that binds the pending challenge to a wrap-safe deadline.
// All timestamps use the same monotonic millis()-style clock.
class TimedPeerHandshakeResponder {
public:
    bool begin(uint32_t nowMs, uint32_t timeoutMs,
               uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
               uint32_t localBootId, const PeerHandshakeHello& hello,
               const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
               const uint8_t* secret, size_t secretLength,
               PeerHandshakeChallenge& challenge);
    bool acceptFinish(uint32_t nowMs, const PeerHandshakeHello& hello,
                      const PeerHandshakeChallenge& challenge,
                      const PeerHandshakeFinish& finish,
                      const uint8_t* secret, size_t secretLength,
                      uint8_t sessionKey[PEER_HANDSHAKE_TAG_SIZE]);
    bool poll(uint32_t nowMs);
    void cancel();
    bool pending() const;
private:
    PeerHandshakeResponder responder_;
    PeerHandshakeTimeout timeout_;
};

} // namespace HWC
#endif
