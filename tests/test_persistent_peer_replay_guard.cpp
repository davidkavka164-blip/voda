#include "PersistentPeerReplayGuard.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace HWC;

class MemoryReplayStore : public IPeerReplayStore {
public:
    MemoryReplayStore() : state(PEER_REPLAY_STORE_EMPTY), failSave(false), saves(0U) {
        memset(&checkpoint, 0, sizeof(checkpoint));
    }
    PeerReplayStoreLoadResult load(PeerReplayCheckpoint& out) {
        if (state == PEER_REPLAY_STORE_VALID) out = checkpoint;
        else memset(&out, 0, sizeof(out));
        return state;
    }
    bool saveAtomically(const PeerReplayCheckpoint& in) {
        if (failSave) return false;
        checkpoint = in;
        state = PEER_REPLAY_STORE_VALID;
        ++saves;
        return true;
    }
    PeerReplayCheckpoint checkpoint;
    PeerReplayStoreLoadResult state;
    bool failSave;
    uint32_t saves;
};

static PeerCommandMetadata command(uint32_t sequence, uint32_t requestId,
                                   uint32_t senderBoot = 0x22223333UL,
                                   uint32_t targetBoot = 0x11112222UL) {
    PeerCommandMetadata c = { 2U, 1U, senderBoot, sequence, requestId, targetBoot };
    return c;
}

int main() {
    MemoryReplayStore store;
    PersistentPeerReplayGuard guard;
    store.failSave = true;
    assert(!guard.beginAuthenticatedSession(1U, 2U, 0x11112222UL,
                                             0x22223333UL, store, 64U));
    // An empty store must be initialized durably before a session becomes active.
    store.failSave = false;
    assert(guard.beginAuthenticatedSession(1U, 2U, 0x11112222UL,
                                            0x22223333UL, store, 64U));
    assert(guard.isActive() && store.saves == 1U);

    store.failSave = true;
    assert(guard.admit(command(1U, 10U), true) == PEER_COMMAND_REJECT_PERSISTENCE_FAILURE);
    assert(guard.lastAcceptedSequence() == 0U && guard.reservedThrough() == 0U);
    store.failSave = false;
    assert(guard.admit(command(1U, 10U), true) == PEER_COMMAND_ACCEPTED);
    assert(guard.lastAcceptedSequence() == 1U && guard.reservedThrough() == 64U);
    assert(store.checkpoint.reservedThrough == 64U);
    assert(guard.admit(command(2U, 11U), true) == PEER_COMMAND_ACCEPTED);
    assert(store.saves == 2U); // one initial write + one reservation, not each packet

    // Simulated receiver reboot: the same peer boot ID must skip the whole
    // previously reserved sequence block, including numbers never actually used.
    guard.endSession();
    PersistentPeerReplayGuard afterReboot;
    assert(afterReboot.beginAuthenticatedSession(1U, 2U, 0x11112223UL,
                                                  0x22223333UL, store, 64U));
    assert(afterReboot.reservedThrough() == 64U);
    assert(afterReboot.admit(command(2U, 11U, 0x22223333UL, 0x11112223UL), true) ==
           PEER_COMMAND_REJECT_REPLAYED_SEQUENCE);
    assert(afterReboot.admit(command(64U, 12U, 0x22223333UL, 0x11112223UL), true) ==
           PEER_COMMAND_REJECT_REPLAYED_SEQUENCE);
    assert(afterReboot.admit(command(65U, 13U, 0x22223333UL, 0x11112223UL), true) ==
           PEER_COMMAND_ACCEPTED);
    assert(afterReboot.reservedThrough() == 128U);

    // Invalid authentication/identity/boot/sequence never advances durable state.
    const uint32_t before = store.checkpoint.reservedThrough;
    assert(afterReboot.admit(command(66U, 14U, 0x22223333UL, 0x11112223UL), false) ==
           PEER_COMMAND_REJECT_UNAUTHENTICATED);
    assert(afterReboot.admit(command(66U, 14U, 0x99999999UL, 0x11112223UL), true) ==
           PEER_COMMAND_REJECT_UNESTABLISHED_SESSION);
    PeerCommandMetadata bad = command(66U, 14U, 0x22223333UL, 0x11112223UL);
    bad.senderDeviceId = 3U;
    assert(afterReboot.admit(bad, true) == PEER_COMMAND_REJECT_WRONG_DEVICE);
    assert(store.checkpoint.reservedThrough == before);

    // A fresh authenticated peer boot epoch may start a new sequence space.
    assert(afterReboot.beginAuthenticatedSession(1U, 2U, 0x11112223UL,
                                                  0x33334444UL, store, 64U));
    assert(afterReboot.reservedThrough() == 0U);
    assert(afterReboot.admit(command(1U, 1U, 0x33334444UL, 0x11112223UL), true) ==
           PEER_COMMAND_ACCEPTED);

    // Corruption and backend errors fail closed; they are never treated as empty.
    MemoryReplayStore corrupt;
    corrupt.state = PEER_REPLAY_STORE_CORRUPT;
    PersistentPeerReplayGuard failClosed;
    assert(!failClosed.beginAuthenticatedSession(1U, 2U, 10U, 20U, corrupt, 64U));
    MemoryReplayStore invalid;
    invalid.state = PEER_REPLAY_STORE_VALID;
    invalid.checkpoint.magic = 123U;
    assert(!failClosed.beginAuthenticatedSession(1U, 2U, 10U, 20U, invalid, 64U));

    // Sequence wrap is not silently accepted; the sender must establish a new boot epoch.
    MemoryReplayStore nearWrap;
    PersistentPeerReplayGuard wrapGuard;
    assert(wrapGuard.beginAuthenticatedSession(1U, 2U, 10U, 20U, nearWrap, 64U));
    assert(wrapGuard.admit(command(0xFFFFFFF0UL, 1U, 20U, 10U), true) ==
           PEER_COMMAND_REJECT_INVALID_ID);
    assert(wrapGuard.admit(command(0x7FFFFFE0UL, 2U, 20U, 10U), true) ==
           PEER_COMMAND_REJECT_INVALID_ID); // reservation would cross epoch limit

    puts("PASS: PersistentPeerReplayGuard reservation, reboot and fail-closed tests");
    return 0;
}
