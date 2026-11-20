#ifndef HWC_PEER_HANDSHAKE_INITIATOR_H
#define HWC_PEER_HANDSHAKE_INITIATOR_H

#include "PeerHandshake.h"
#include "PeerHandshakeReady.h"
#include "PeerSequenceAllocator.h"

namespace HWC {

enum PeerHandshakeInitiatorState : uint8_t {
    PEER_HS_INIT_IDLE = 0U,
    PEER_HS_INIT_WAIT_CHALLENGE = 1U,
    PEER_HS_INIT_WAIT_READY = 2U,
    PEER_HS_INIT_ESTABLISHED = 3U,
    PEER_HS_INIT_FAILED = 4U
};

// Transport-independent initiator coordinator. The transport sends the
// returned HELLO/FINISH bytes and supplies CHALLENGE/READY after wire decoding.
// Command traffic is authorized only after READY has been authenticated.
class PeerHandshakeInitiator {
public:
    PeerHandshakeInitiator();
    ~PeerHandshakeInitiator();
    PeerHandshakeInitiator(const PeerHandshakeInitiator&) = delete;
    PeerHandshakeInitiator& operator=(const PeerHandshakeInitiator&) = delete;

    bool begin(const PeerHandshakeHello& hello, const uint8_t* secret,
               size_t secretLength, PeerSequenceAllocator& sequenceAllocator);
    bool hello(PeerHandshakeHello& out) const;
    bool acceptChallenge(const PeerHandshakeChallenge& challenge,
                         PeerHandshakeFinish& finish);
    // FINISH is cached: a transport retry must send these exact bytes again.
    bool finishForRetry(PeerHandshakeFinish& out) const;
    bool acceptReady(const PeerHandshakeReady& ready);
    bool established() const;
    PeerHandshakeInitiatorState state() const;
    bool copySessionKey(uint8_t out[PEER_HANDSHAKE_TAG_SIZE]) const;
    uint32_t authenticatedReplayFloor() const;
    void cancel();

private:
    PeerHandshakeHello hello_;
    PeerHandshakeFinish finish_;
    uint8_t secret_[64U];
    size_t secretLength_;
    uint8_t sessionKey_[PEER_HANDSHAKE_TAG_SIZE];
    uint32_t replayFloor_;
    PeerSequenceAllocator* sequenceAllocator_;
    PeerHandshakeInitiatorState state_;
};

} // namespace HWC
#endif
