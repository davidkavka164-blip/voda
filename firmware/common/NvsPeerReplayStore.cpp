#include "NvsPeerReplayStore.h"
#include <string.h>

namespace HWC {
namespace {
const char* slotKey(uint8_t slot) { return slot == 0U ? "slot0" : "slot1"; }
const char* kInitMarkerKey = "init";
const uint8_t kInitMarkerValue = 0xA7U;

bool sameCheckpoint(const PeerReplayCheckpoint& a, const PeerReplayCheckpoint& b) {
    return a.magic == b.magic && a.version == b.version &&
           a.localDeviceId == b.localDeviceId && a.peerDeviceId == b.peerDeviceId &&
           a.peerBootId == b.peerBootId && a.reservedThrough == b.reservedThrough &&
           a.generation == b.generation && a.crc32 == b.crc32;
}
}

NvsPeerReplayStore::NvsPeerReplayStore(const char* nameSpace)
    : nameSpace_(nameSpace), opened_(false) {}

bool NvsPeerReplayStore::begin() {
    if (opened_) return true;
    opened_ = nameSpace_ && nameSpace_[0] && strlen(nameSpace_) <= 15U &&
              preferences_.begin(nameSpace_, false);
    return opened_;
}

void NvsPeerReplayStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsPeerReplayStore::isOpen() const { return opened_; }

PeerReplayStoreLoadResult NvsPeerReplayStore::readSlot(
    uint8_t slot, PeerReplayCheckpoint& checkpoint) {
    memset(&checkpoint, 0, sizeof(checkpoint));
    if (!opened_ || slot > 1U) return PEER_REPLAY_STORE_ERROR;
    const char* key = slotKey(slot);
    if (!preferences_.isKey(key)) return PEER_REPLAY_STORE_EMPTY;
    if (preferences_.getBytesLength(key) != sizeof(PeerReplayCheckpoint))
        return PEER_REPLAY_STORE_CORRUPT;
    if (preferences_.getBytes(key, &checkpoint, sizeof(checkpoint)) != sizeof(checkpoint))
        return PEER_REPLAY_STORE_ERROR;
    return PersistentPeerReplayGuard::checkpointValid(checkpoint)
               ? PEER_REPLAY_STORE_VALID : PEER_REPLAY_STORE_CORRUPT;
}

bool NvsPeerReplayStore::newerGeneration(uint32_t a, uint32_t b) {
    return static_cast<int32_t>(a - b) > 0;
}

PeerReplayStoreLoadResult NvsPeerReplayStore::load(PeerReplayCheckpoint& checkpoint) {
    memset(&checkpoint, 0, sizeof(checkpoint));
    if (!opened_) return PEER_REPLAY_STORE_ERROR;

    PeerReplayCheckpoint a = {}, b = {};
    const PeerReplayStoreLoadResult ra = readSlot(0U, a);
    const PeerReplayStoreLoadResult rb = readSlot(1U, b);
    if (ra == PEER_REPLAY_STORE_ERROR || rb == PEER_REPLAY_STORE_ERROR)
        return PEER_REPLAY_STORE_ERROR;
    // Anti-replay state must never roll back to an older slot when either
    // journal entry is corrupt. A torn/failed NVS update is handled by the
    // storage layer's atomic blob semantics; corruption is not a safe signal
    // that the other slot is the newest committed security state.
    if (ra == PEER_REPLAY_STORE_CORRUPT || rb == PEER_REPLAY_STORE_CORRUPT)
        return PEER_REPLAY_STORE_CORRUPT;
    const bool markerPresent = preferences_.isKey(kInitMarkerKey);
    if (markerPresent && preferences_.getUChar(kInitMarkerKey, 0U) != kInitMarkerValue)
        return PEER_REPLAY_STORE_CORRUPT;
    if (ra == PEER_REPLAY_STORE_VALID && rb == PEER_REPLAY_STORE_VALID) {
        if (a.generation == b.generation) {
            if (!sameCheckpoint(a, b)) return PEER_REPLAY_STORE_CORRUPT;
            checkpoint = a;
        } else {
            checkpoint = newerGeneration(b.generation, a.generation) ? b : a;
        }
        return PEER_REPLAY_STORE_VALID;
    }
    if (ra == PEER_REPLAY_STORE_VALID) { checkpoint = a; return PEER_REPLAY_STORE_VALID; }
    if (rb == PEER_REPLAY_STORE_VALID) { checkpoint = b; return PEER_REPLAY_STORE_VALID; }
    return markerPresent ? PEER_REPLAY_STORE_CORRUPT : PEER_REPLAY_STORE_EMPTY;
}

bool NvsPeerReplayStore::saveAtomically(const PeerReplayCheckpoint& checkpoint) {
    if (!opened_ || !PersistentPeerReplayGuard::checkpointValid(checkpoint)) return false;

    PeerReplayCheckpoint current = {};
    const PeerReplayStoreLoadResult state = load(current);
    if (state == PEER_REPLAY_STORE_CORRUPT || state == PEER_REPLAY_STORE_ERROR) return false;

    uint8_t targetSlot = 0U;
    uint32_t generation = 1U;
    if (state == PEER_REPLAY_STORE_VALID) {
        PeerReplayCheckpoint a = {}, b = {};
        const PeerReplayStoreLoadResult ra = readSlot(0U, a);
        const PeerReplayStoreLoadResult rb = readSlot(1U, b);
        const uint8_t currentSlot = (ra == PEER_REPLAY_STORE_VALID &&
            (rb != PEER_REPLAY_STORE_VALID || !newerGeneration(b.generation, a.generation))) ? 0U : 1U;
        targetSlot = static_cast<uint8_t>(1U - currentSlot);
        generation = current.generation + 1U;
    }

    PeerReplayCheckpoint next = checkpoint;
    next.generation = generation;
    next.crc32 = PersistentPeerReplayGuard::checkpointCrc32(next);
    if (preferences_.putBytes(slotKey(targetSlot), &next, sizeof(next)) != sizeof(next))
        return false;

    PeerReplayCheckpoint verify = {};
    const PeerReplayStoreLoadResult verifyState = load(verify);
    if (verifyState != PEER_REPLAY_STORE_VALID || !sameCheckpoint(verify, next)) return false;
    // Commit the initialization marker only after a valid first checkpoint is
    // readable. If power fails before this write, the checkpoint itself allows
    // recovery; once the marker exists, losing both slots is corruption.
    if (!preferences_.isKey(kInitMarkerKey) &&
        !preferences_.putUChar(kInitMarkerKey, kInitMarkerValue)) return false;
    return preferences_.getUChar(kInitMarkerKey, 0U) == kInitMarkerValue;
}

} // namespace HWC
