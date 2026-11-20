#ifndef HWC_PERSISTENT_PEER_REPLAY_GUARD_H
#define HWC_PERSISTENT_PEER_REPLAY_GUARD_H

#include <stdint.h>
#include "PeerCommandGuard.h"

namespace HWC {

enum PeerReplayStoreLoadResult {
    PEER_REPLAY_STORE_EMPTY,
    PEER_REPLAY_STORE_VALID,
    PEER_REPLAY_STORE_CORRUPT,
    PEER_REPLAY_STORE_ERROR
};

struct PeerReplayCheckpoint {
    uint32_t magic;
    uint16_t version;
    uint8_t localDeviceId;
    uint8_t peerDeviceId;
    uint32_t peerBootId;
    uint32_t reservedThrough;
    uint32_t generation;
    uint32_t crc32;
};

// Implementations must provide atomic replacement: after power loss load() must
// return either the previous complete checkpoint or the new complete checkpoint.
// CRC detects accidental corruption; it is NOT protection against flash rollback
// or a physical attacker. A real ESP32 backend is intentionally not supplied yet.
class IPeerReplayStore {
public:
    virtual ~IPeerReplayStore() {}
    virtual PeerReplayStoreLoadResult load(PeerReplayCheckpoint& checkpoint) = 0;
    virtual bool saveAtomically(const PeerReplayCheckpoint& checkpoint) = 0;
};

// Read the receiver-side floor before issuing an authenticated CHALLENGE. The
// returned value must be copied into PeerHandshakeChallenge so its HMAC binds
// the floor to this exact peer boot epoch. Corrupt/error storage fails closed.
bool readPeerReplayFloorForHandshake(IPeerReplayStore& store,
                                     uint8_t localDeviceId, uint8_t peerDeviceId,
                                     uint32_t authenticatedPeerBootId,
                                     uint32_t& replayFloor);

class PersistentPeerReplayGuard {
public:
    PersistentPeerReplayGuard();
    PersistentPeerReplayGuard(const PersistentPeerReplayGuard&) = delete;
    PersistentPeerReplayGuard& operator=(const PersistentPeerReplayGuard&) = delete;

    // Must be called only after a fresh authenticated handshake. If the peer boot
    // ID is unchanged, the reserved sequence floor is preserved across local reboot.
    // A changed peer boot ID starts a new sequence epoch only after that handshake.
    bool beginAuthenticatedSession(uint8_t localDeviceId, uint8_t peerDeviceId,
                                   uint32_t localBootId, uint32_t peerBootId,
                                   IPeerReplayStore& store,
                                   uint32_t reservationWindow);
    void endSession();
    PeerCommandAdmission admit(const PeerCommandMetadata& command,
                               bool frameAuthenticated);
    bool isActive() const;
    uint32_t lastAcceptedSequence() const;
    uint32_t reservedThrough() const;

    static uint32_t checkpointCrc32(const PeerReplayCheckpoint& checkpoint);
    static bool checkpointValid(const PeerReplayCheckpoint& checkpoint);

private:
    IPeerReplayStore* store_;
    uint8_t localDeviceId_;
    uint8_t peerDeviceId_;
    uint32_t localBootId_;
    uint32_t peerBootId_;
    uint32_t lastAcceptedSequence_;
    uint32_t reservedThrough_;
    uint32_t reservationWindow_;
    bool active_;

    bool persistReservation(uint32_t newReservedThrough);
};

} // namespace HWC
#endif
