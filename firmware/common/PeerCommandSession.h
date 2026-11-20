#ifndef HWC_PEER_COMMAND_SESSION_H
#define HWC_PEER_COMMAND_SESSION_H

#include "PeerCommandGuard.h"
#include "PeerSessionCodec.h"
#include "PeerRequestTracker.h"
#include "PersistentPeerReplayGuard.h"
#include "PeerSequenceAllocator.h"

namespace HWC {

// Couples one authenticated cryptographic session to command admission.
// activate() MUST only be called after a fresh HELLO/CHALLENGE/FINISH handshake
// has been fully authenticated and directional keys have been derived.
// This class is transport-independent and does not execute commands.
class PeerCommandSession {
public:
    PeerCommandSession();
    PeerCommandSession(const PeerCommandSession&) = delete;
    PeerCommandSession& operator=(const PeerCommandSession&) = delete;

    bool activate(const PeerDirectionalKeys& keys,
                  uint8_t localDeviceId, uint8_t peerDeviceId,
                  uint32_t localBootId, uint32_t peerBootId,
                  bool localIsInitiator, bool freshHandshakeAuthenticated);
    // Persistent anti-replay variant for the receiving side. The supplied
    // NVS journal is a platform backend; physical power-cut behavior still
    // requires validation on the exact ESP32/Arduino-ESP32 target.
    bool activateWithPersistentReplay(const PeerDirectionalKeys& keys,
                                      uint8_t localDeviceId, uint8_t peerDeviceId,
                                      uint32_t localBootId, uint32_t peerBootId,
                                      bool localIsInitiator,
                                      bool freshHandshakeAuthenticated,
                                      IPeerReplayStore& replayStore,
                                      uint32_t reservationWindow);
    void close();
    bool isActive() const;

    bool encodeOutgoing(const PeerCommandFrame& frame,
                        uint8_t* output, size_t capacity, size_t& written) const;

    // Allocates the next sender sequence, prepares an authenticated COMMAND
    // frame and registers its correlation before the caller transmits bytes.
    // Use this for production COMMAND transmission; encodeOutgoing() is the
    // lower-level codec helper and does not allocate or track a sequence.
    // If registration fails, no bytes are exposed for transmission.
    PeerRequestResult prepareOutgoingRequest(const PeerCommandFrame& request,
                                             uint32_t nowMs, uint32_t timeoutMs,
                                             uint8_t* output, size_t capacity,
                                             size_t& written);

    // Decodes and authenticates a RESPONSE with this session's directional
    // key before allowing the tracker to correlate it with a pending request.
    PeerRequestResult receiveResponse(const uint8_t* data, size_t length,
                                      uint32_t nowMs,
                                      PeerCommandResponse& response);
    bool takeExpiredRequest(uint32_t nowMs, uint32_t& requestId);
    size_t pendingRequestCount() const;
    // Sequence floor that the authenticated peer sender must exceed before
    // sending commands after a receiver reboot/session re-establishment.
    uint32_t persistentReplayFloor() const;

    // Apply only a floor obtained from the authenticated peer session-sync
    // exchange. The caller is responsible for authenticating that exchange.
    bool synchronizeOutgoingSequenceFloor(uint32_t authenticatedReservedFloor);
    uint32_t nextOutgoingSequence() const;

    // Only authenticated COMMAND frames are admitted here. RESPONSE frames
    // are intentionally left for a separate request-correlation layer.
    PeerCommandAdmission receiveCommand(const uint8_t* data, size_t length,
                                        PeerCommandFrame& acceptedCommand);

    uint32_t acceptedCommandCount() const;
    uint32_t peerBootId() const;

private:
    PeerSessionCodec codec_;
    PeerCommandGuard guard_;
    PeerRequestTracker requestTracker_;
    PersistentPeerReplayGuard persistentReplayGuard_;
    PeerSequenceAllocator outgoingSequenceAllocator_;
    bool persistentReplayEnabled_;
    bool active_;
    uint8_t localDeviceId_;
    uint32_t localBootId_;
};

} // namespace HWC
#endif
