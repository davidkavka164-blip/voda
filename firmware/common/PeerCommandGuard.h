#ifndef HWC_PEER_COMMAND_GUARD_H
#define HWC_PEER_COMMAND_GUARD_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

enum PeerCommandAdmission {
    PEER_COMMAND_ACCEPTED,
    PEER_COMMAND_REJECT_UNAUTHENTICATED,
    PEER_COMMAND_REJECT_WRONG_TARGET_BOOT,
    PEER_COMMAND_REJECT_WRONG_DEVICE,
    PEER_COMMAND_REJECT_UNESTABLISHED_SESSION,
    PEER_COMMAND_REJECT_INVALID_ID,
    PEER_COMMAND_REJECT_REPLAYED_SEQUENCE,
    PEER_COMMAND_REJECT_DUPLICATE_REQUEST,
    PEER_COMMAND_REJECT_WRONG_FRAME_TYPE,
    PEER_COMMAND_REJECT_PERSISTENCE_FAILURE
};

struct PeerCommandMetadata {
    uint8_t senderDeviceId;
    uint8_t targetDeviceId;
    uint32_t senderBootId;
    uint32_t senderSequence;
    uint32_t requestId;
    uint32_t targetBootId;
};

// Admission/replay guard. It does not perform cryptographic authentication.
// recordAuthenticatedPeerSession() may only be called after a fresh, authenticated
// peer handshake has been verified by a separate cryptographic/session layer.
class PeerCommandGuard {
public:
    static const size_t REQUEST_CACHE_SIZE = 16;
    PeerCommandGuard();
    void reset(uint8_t localDeviceId, uint32_t localBootId);
    void endSession();
    bool recordAuthenticatedPeerSession(uint8_t peerDeviceId, uint32_t peerBootId,
                                        bool freshHandshakeAuthenticated);
    PeerCommandAdmission admit(const PeerCommandMetadata& command, bool frameAuthenticated);
    uint32_t acceptedCount() const;
    uint32_t peerBootId() const;
private:
    uint8_t localDeviceId_;
    uint8_t peerDeviceId_;
    uint32_t localBootId_;
    uint32_t peerBootId_;
    uint32_t lastSequence_;
    uint32_t requestIds_[REQUEST_CACHE_SIZE];
    size_t nextRequestIndex_;
    size_t requestCount_;
    uint32_t acceptedCount_;
    bool sessionEstablished_;
    bool hasRequestId(uint32_t requestId) const;
    void rememberRequestId(uint32_t requestId);
    void clearReplayState();
};

} // namespace HWC
#endif
