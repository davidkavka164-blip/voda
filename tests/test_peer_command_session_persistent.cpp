#include "PeerCommandSession.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace HWC;

class MemoryReplayStore : public IPeerReplayStore {
public:
    MemoryReplayStore() : state(PEER_REPLAY_STORE_EMPTY) { memset(&cp, 0, sizeof(cp)); }
    PeerReplayStoreLoadResult load(PeerReplayCheckpoint& out) {
        if (state == PEER_REPLAY_STORE_VALID) out = cp;
        else memset(&out, 0, sizeof(out));
        return state;
    }
    bool saveAtomically(const PeerReplayCheckpoint& in) {
        cp = in; state = PEER_REPLAY_STORE_VALID; return true;
    }
    PeerReplayCheckpoint cp;
    PeerReplayStoreLoadResult state;
};

static PeerCommandFrame makeCommand(uint32_t senderBoot, uint32_t targetBoot,
                                    uint32_t sequence, uint32_t requestId) {
    PeerCommandFrame f;
    memset(&f, 0, sizeof(f));
    f.frameType = PEER_FRAME_COMMAND;
    f.commandCode = 4U;
    f.senderDeviceId = 1U;
    f.targetDeviceId = 2U;
    f.senderBootId = senderBoot;
    f.targetBootId = targetBoot;
    f.sequence = sequence;
    f.requestId = requestId;
    return f;
}

static void derive(uint32_t initiatorBoot, uint32_t responderBoot,
                   PeerDirectionalKeys& keys) {
    uint8_t sessionKey[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(sessionKey); ++i)
        sessionKey[i] = static_cast<uint8_t>(0xA0U + i);
    assert(derivePeerDirectionalKeys(sessionKey, sizeof(sessionKey), 1U, 2U,
                                     initiatorBoot, responderBoot, keys));
    memset(sessionKey, 0, sizeof(sessionKey));
}

int main() {
    const uint32_t senderBoot = 0x11112222UL;
    const uint32_t receiverBoot1 = 0x33334444UL;
    const uint32_t receiverBoot2 = 0x33334445UL;
    MemoryReplayStore store;
    PeerDirectionalKeys keys1;
    derive(senderBoot, receiverBoot1, keys1);
    PeerCommandSession sender1, receiver1;
    assert(sender1.activate(keys1, 1U, 2U, senderBoot, receiverBoot1, true, true));
    assert(receiver1.activateWithPersistentReplay(keys1, 2U, 1U,
            receiverBoot1, senderBoot, false, true, store, 64U));

    uint8_t wire[PEER_COMMAND_FRAME_SIZE];
    size_t written = 0U;
    PeerCommandFrame accepted;
    PeerCommandFrame f = makeCommand(senderBoot, receiverBoot1, 1U, 100U);
    assert(sender1.encodeOutgoing(f, wire, sizeof(wire), written));
    assert(receiver1.receiveCommand(wire, written, accepted) == PEER_COMMAND_ACCEPTED);
    assert(store.cp.reservedThrough == 64U);
    receiver1.close(); // simulated power cycle of receiver

    PeerDirectionalKeys keys2;
    derive(senderBoot, receiverBoot2, keys2);
    PeerCommandSession sender2, receiver2;
    assert(sender2.activate(keys2, 1U, 2U, senderBoot, receiverBoot2, true, true));
    assert(receiver2.activateWithPersistentReplay(keys2, 2U, 1U,
            receiverBoot2, senderBoot, false, true, store, 64U));
    assert(receiver2.persistentReplayFloor() == 64U);

    // Sender synchronizes to the receiver's authenticated persistent floor.
    assert(sender2.synchronizeOutgoingSequenceFloor(receiver2.persistentReplayFloor()));
    assert(sender2.nextOutgoingSequence() == 65U);
    f = makeCommand(senderBoot, receiverBoot2, 0U, 102U);
    assert(sender2.prepareOutgoingRequest(f, 100U, 5000U, wire, sizeof(wire), written)
           == PEER_REQUEST_TRACKED);
    assert(receiver2.receiveCommand(wire, written, accepted) == PEER_COMMAND_ACCEPTED);
    assert(accepted.sequence == 65U && accepted.requestId == 102U);
    assert(store.cp.reservedThrough == 128U);

    clearPeerDirectionalKeys(keys1);
    clearPeerDirectionalKeys(keys2);
    puts("PASS: PeerCommandSession persistent anti-replay integration");
    return 0;
}
