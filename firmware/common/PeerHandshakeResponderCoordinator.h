#ifndef HWC_PEER_HANDSHAKE_RESPONDER_COORDINATOR_H
#define HWC_PEER_HANDSHAKE_RESPONDER_COORDINATOR_H

#include "PeerHandshake.h"
#include "PeerHandshakeReady.h"
#include "PeerHandshakeTimeout.h"
#include "PersistentPeerReplayGuard.h"

namespace HWC {

enum PeerHandshakeResponderState : uint8_t {
    PEER_HS_RESP_IDLE = 0U,
    PEER_HS_RESP_WAIT_FINISH = 1U,
    PEER_HS_RESP_READY_PENDING = 2U,
    PEER_HS_RESP_ESTABLISHED = 3U,
    PEER_HS_RESP_FAILED = 4U
};

// Transport-independent responder lifecycle. READY is cached byte-for-byte
// for retries. The session key is withheld until the transport reports that
// READY was accepted for transmission; the initiator independently verifies it.
class PeerHandshakeResponderCoordinator {
public:
    PeerHandshakeResponderCoordinator();
    ~PeerHandshakeResponderCoordinator();
    PeerHandshakeResponderCoordinator(const PeerHandshakeResponderCoordinator&) = delete;
    PeerHandshakeResponderCoordinator& operator=(const PeerHandshakeResponderCoordinator&) = delete;

    bool begin(uint32_t nowMs, uint32_t timeoutMs,
               uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
               uint32_t localBootId, const PeerHandshakeHello& hello,
               const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
               const uint8_t* secret, size_t secretLength,
               IPeerReplayStore& replayStore, PeerHandshakeChallenge& challenge);
    bool acceptFinish(uint32_t nowMs, const PeerHandshakeHello& hello,
                      const PeerHandshakeChallenge& challenge,
                      const PeerHandshakeFinish& finish,
                      const uint8_t* secret, size_t secretLength,
                      PeerHandshakeReady& ready);
    bool readyForRetry(PeerHandshakeReady& ready) const;
    bool markReadyTransmitted(uint32_t nowMs);
    bool poll(uint32_t nowMs);
    bool copySessionKey(uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const;
    PeerHandshakeResponderState state() const;
    void cancel();

private:
    PeerHandshakeResponder responder_;
    PeerHandshakeTimeout timeout_;
    PeerHandshakeReady ready_;
    PeerHandshakeFinish acceptedFinish_;
    uint8_t sessionKey_[PEER_HANDSHAKE_TAG_SIZE];
    PeerHandshakeResponderState state_;
};

} // namespace HWC
#endif
