#include "PersistentPeerReplayGuard.h"
#include <string.h>

namespace HWC {
namespace {
static const uint32_t CHECKPOINT_MAGIC = 0x48575250UL; // "HWRP"
static const uint16_t CHECKPOINT_VERSION = 2U;

static void crcByte(uint32_t& crc, uint8_t value) {
    crc ^= value;
    for (uint8_t bit = 0; bit < 8U; ++bit)
        crc = (crc & 1U) ? ((crc >> 1U) ^ 0xEDB88320UL) : (crc >> 1U);
}
static void crc16(uint32_t& crc, uint16_t value) {
    crcByte(crc, static_cast<uint8_t>(value));
    crcByte(crc, static_cast<uint8_t>(value >> 8U));
}
static void crc32le(uint32_t& crc, uint32_t value) {
    for (uint8_t i = 0; i < 4U; ++i) crcByte(crc, static_cast<uint8_t>(value >> (8U * i)));
}
}

bool readPeerReplayFloorForHandshake(IPeerReplayStore& store,
                                     uint8_t localDeviceId, uint8_t peerDeviceId,
                                     uint32_t authenticatedPeerBootId,
                                     uint32_t& replayFloor) {
    replayFloor = 0U;
    if (localDeviceId == 0U || peerDeviceId == 0U ||
        localDeviceId == peerDeviceId || authenticatedPeerBootId == 0U)
        return false;
    PeerReplayCheckpoint checkpoint;
    memset(&checkpoint, 0, sizeof(checkpoint));
    const PeerReplayStoreLoadResult result = store.load(checkpoint);
    if (result == PEER_REPLAY_STORE_EMPTY) return true;
    if (result != PEER_REPLAY_STORE_VALID ||
        !PersistentPeerReplayGuard::checkpointValid(checkpoint) ||
        checkpoint.localDeviceId != localDeviceId ||
        checkpoint.peerDeviceId != peerDeviceId) return false;
    if (checkpoint.peerBootId == authenticatedPeerBootId)
        replayFloor = checkpoint.reservedThrough;
    return true;
}

PersistentPeerReplayGuard::PersistentPeerReplayGuard()
    : store_(0), localDeviceId_(0U), peerDeviceId_(0U), localBootId_(0U),
      peerBootId_(0U), lastAcceptedSequence_(0U), reservedThrough_(0U),
      reservationWindow_(0U), active_(false) {}

uint32_t PersistentPeerReplayGuard::checkpointCrc32(const PeerReplayCheckpoint& cp) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc32le(crc, cp.magic);
    crc16(crc, cp.version);
    crcByte(crc, cp.localDeviceId);
    crcByte(crc, cp.peerDeviceId);
    crc32le(crc, cp.peerBootId);
    crc32le(crc, cp.reservedThrough);
    crc32le(crc, cp.generation);
    return crc ^ 0xFFFFFFFFUL;
}

bool PersistentPeerReplayGuard::checkpointValid(const PeerReplayCheckpoint& cp) {
    return cp.magic == CHECKPOINT_MAGIC && cp.version == CHECKPOINT_VERSION &&
           cp.localDeviceId != 0U && cp.peerDeviceId != 0U &&
           cp.localDeviceId != cp.peerDeviceId && cp.peerBootId != 0U &&
           cp.reservedThrough < 0x80000000UL &&
           cp.crc32 == checkpointCrc32(cp);
}

bool PersistentPeerReplayGuard::beginAuthenticatedSession(
    uint8_t localDeviceId, uint8_t peerDeviceId, uint32_t localBootId,
    uint32_t peerBootId, IPeerReplayStore& store, uint32_t reservationWindow) {
    endSession();
    if (localDeviceId == 0U || peerDeviceId == 0U || localDeviceId == peerDeviceId ||
        localBootId == 0U || peerBootId == 0U || reservationWindow == 0U ||
        reservationWindow > 65536U) return false;

    PeerReplayCheckpoint cp;
    memset(&cp, 0, sizeof(cp));
    const PeerReplayStoreLoadResult loadResult = store.load(cp);
    if (loadResult == PEER_REPLAY_STORE_CORRUPT || loadResult == PEER_REPLAY_STORE_ERROR)
        return false;

    uint32_t floor = 0U;
    bool mustSave = false;
    if (loadResult == PEER_REPLAY_STORE_VALID) {
        if (!checkpointValid(cp) || cp.localDeviceId != localDeviceId ||
            cp.peerDeviceId != peerDeviceId) return false;
        if (cp.peerBootId == peerBootId) {
            floor = cp.reservedThrough;
        } else {
            // A new peer boot epoch is accepted only after the caller has verified
            // a fresh challenge-response handshake for this exact peer boot ID.
            floor = 0U;
            mustSave = true;
        }
    } else if (loadResult == PEER_REPLAY_STORE_EMPTY) {
        mustSave = true;
    } else {
        return false;
    }

    if (mustSave) {
        memset(&cp, 0, sizeof(cp));
        cp.magic = CHECKPOINT_MAGIC;
        cp.version = CHECKPOINT_VERSION;
        cp.localDeviceId = localDeviceId;
        cp.peerDeviceId = peerDeviceId;
        cp.peerBootId = peerBootId;
        cp.reservedThrough = 0U;
        cp.crc32 = checkpointCrc32(cp);
        if (!store.saveAtomically(cp)) return false;
    }

    store_ = &store;
    localDeviceId_ = localDeviceId;
    peerDeviceId_ = peerDeviceId;
    localBootId_ = localBootId;
    peerBootId_ = peerBootId;
    lastAcceptedSequence_ = floor;
    reservedThrough_ = floor;
    reservationWindow_ = reservationWindow;
    active_ = true;
    return true;
}

void PersistentPeerReplayGuard::endSession() {
    store_ = 0;
    localDeviceId_ = peerDeviceId_ = 0U;
    localBootId_ = peerBootId_ = 0U;
    lastAcceptedSequence_ = reservedThrough_ = reservationWindow_ = 0U;
    active_ = false;
}

bool PersistentPeerReplayGuard::persistReservation(uint32_t newReservedThrough) {
    if (!active_ || store_ == 0 || newReservedThrough < reservedThrough_) return false;
    PeerReplayCheckpoint cp;
    memset(&cp, 0, sizeof(cp));
    cp.magic = CHECKPOINT_MAGIC;
    cp.version = CHECKPOINT_VERSION;
    cp.localDeviceId = localDeviceId_;
    cp.peerDeviceId = peerDeviceId_;
    cp.peerBootId = peerBootId_;
    cp.reservedThrough = newReservedThrough;
    cp.crc32 = checkpointCrc32(cp);
    if (!store_->saveAtomically(cp)) return false;
    reservedThrough_ = newReservedThrough;
    return true;
}

PeerCommandAdmission PersistentPeerReplayGuard::admit(
    const PeerCommandMetadata& command, bool frameAuthenticated) {
    if (!frameAuthenticated) return PEER_COMMAND_REJECT_UNAUTHENTICATED;
    if (!active_) return PEER_COMMAND_REJECT_UNESTABLISHED_SESSION;
    if (command.senderDeviceId == 0U || command.targetDeviceId == 0U ||
        command.senderBootId == 0U || command.senderSequence == 0U ||
        command.requestId == 0U || command.targetBootId == 0U)
        return PEER_COMMAND_REJECT_INVALID_ID;
    if (command.senderDeviceId != peerDeviceId_ || command.targetDeviceId != localDeviceId_)
        return PEER_COMMAND_REJECT_WRONG_DEVICE;
    if (command.targetBootId != localBootId_) return PEER_COMMAND_REJECT_WRONG_TARGET_BOOT;
    if (command.senderBootId != peerBootId_) return PEER_COMMAND_REJECT_UNESTABLISHED_SESSION;
    // Keep each boot epoch within the unambiguous half of the uint32 sequence space.
    // A new authenticated peer boot epoch is required before reaching 2^31.
    if (command.senderSequence >= 0x80000000UL) return PEER_COMMAND_REJECT_INVALID_ID;
    if (static_cast<int32_t>(command.senderSequence - lastAcceptedSequence_) <= 0)
        return PEER_COMMAND_REJECT_REPLAYED_SEQUENCE;

    if (command.senderSequence > reservedThrough_) {
        const uint64_t blocks =
            (static_cast<uint64_t>(command.senderSequence) + reservationWindow_ - 1U) /
            reservationWindow_;
        const uint64_t nextFloor = blocks * reservationWindow_;
        if (nextFloor >= 0x80000000ULL ||
            static_cast<uint64_t>(command.senderSequence) - lastAcceptedSequence_ >= 0x80000000ULL)
            return PEER_COMMAND_REJECT_INVALID_ID;
        if (!persistReservation(static_cast<uint32_t>(nextFloor)))
            return PEER_COMMAND_REJECT_PERSISTENCE_FAILURE;
    }
    lastAcceptedSequence_ = command.senderSequence;
    return PEER_COMMAND_ACCEPTED;
}

bool PersistentPeerReplayGuard::isActive() const { return active_; }
uint32_t PersistentPeerReplayGuard::lastAcceptedSequence() const { return lastAcceptedSequence_; }
uint32_t PersistentPeerReplayGuard::reservedThrough() const { return reservedThrough_; }

} // namespace HWC
