#include "NvsPeerReplayStore.h"
#include "PeerHandshake.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

unsigned char Preferences::data[2][256] = {{0}};
size_t Preferences::lengths[2] = {0,0};
bool Preferences::present[2] = {false,false};
bool Preferences::failWrite = false;
bool Preferences::markerPresent = false;
uint8_t Preferences::marker = 0;
char Preferences::lastNamespace[32] = {0};

using namespace HWC;
static PeerReplayCheckpoint makeCheckpoint(uint32_t floor) {
    PeerReplayCheckpoint cp = {};
    cp.magic = 0x48575250UL; cp.version = 2U;
    cp.localDeviceId = 1U; cp.peerDeviceId = 2U;
    cp.peerBootId = 0x12345678UL; cp.reservedThrough = floor;
    cp.crc32 = PersistentPeerReplayGuard::checkpointCrc32(cp);
    return cp;
}

int main() {
    Preferences::clearForTest();
    NvsPeerReplayStore store;
    assert(store.begin() && store.isOpen());
    assert(strcmp(Preferences::lastNamespace, "hwc_peer_rp") == 0);
    PeerReplayCheckpoint out = {};
    assert(store.load(out) == PEER_REPLAY_STORE_EMPTY);

    assert(store.saveAtomically(makeCheckpoint(64U)));
    assert(Preferences::present[0] && !Preferences::present[1]);
    assert(store.load(out) == PEER_REPLAY_STORE_VALID && out.reservedThrough == 64U && out.generation == 1U);

    assert(store.saveAtomically(makeCheckpoint(128U)));
    assert(Preferences::present[0] && Preferences::present[1]);
    assert(store.load(out) == PEER_REPLAY_STORE_VALID && out.reservedThrough == 128U && out.generation == 2U);
    uint32_t handshakeFloor = 0U;
    assert(readPeerReplayFloorForHandshake(store, 1U, 2U, 0x12345678UL, handshakeFloor));
    assert(handshakeFloor == 128U);
    assert(readPeerReplayFloorForHandshake(store, 1U, 2U, 0x87654321UL, handshakeFloor));
    assert(handshakeFloor == 0U); // new authenticated peer boot epoch
    assert(!readPeerReplayFloorForHandshake(store, 1U, 3U, 0x12345678UL, handshakeFloor));

    // Receiver-side handshake reads the persisted floor and binds it into the
    // HMAC-authenticated CHALLENGE, without trusting a caller-supplied floor.
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 2U; hello.responderDeviceId = 1U;
    hello.initiatorBootId = 0x12345678UL;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);
    uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    uint8_t secret[32];
    for (size_t i = 0; i < sizeof(responderNonce); ++i)
        responderNonce[i] = static_cast<uint8_t>(0xA0U + i);
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 7U);
    PeerHandshakeResponder responder;
    PeerHandshakeChallenge challenge = {};
    assert(responder.beginWithReplayStore(2U, 1U, 0x11111111UL, hello,
        responderNonce, secret, sizeof(secret), store, challenge));
    assert(challenge.authenticatedReplayFloor == 128U);
    PeerHandshakeFinish finish = {};
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                        finish, initiatorKey));
    challenge.authenticatedReplayFloor++;
    uint8_t rejectedKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(!verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                         finish, rejectedKey));
    responder.cancel();

    // Simulate power loss before the replacement slot is committed: the previous
    // complete checkpoint remains readable and no higher sequence is forgotten.
    Preferences::failWrite = true;
    assert(!store.saveAtomically(makeCheckpoint(192U)));
    Preferences::failWrite = false;
    assert(store.load(out) == PEER_REPLAY_STORE_VALID && out.reservedThrough == 128U);

    // A damaged newest slot must fail closed. Falling back to the older
    // checkpoint would roll back the replay floor and could admit old packets.
    Preferences::data[1][0] ^= 0x80U;
    assert(store.load(out) == PEER_REPLAY_STORE_CORRUPT);
    assert(!store.saveAtomically(makeCheckpoint(192U)));
    assert(!readPeerReplayFloorForHandshake(store, 1U, 2U, 0x12345678UL, handshakeFloor));

    // Once initialized, loss of both journal slots must not look like first boot.
    Preferences::clearForTest();
    assert(store.saveAtomically(makeCheckpoint(64U)));
    assert(Preferences::markerPresent);
    Preferences::present[0] = false;
    Preferences::present[1] = false;
    assert(store.load(out) == PEER_REPLAY_STORE_CORRUPT);

    // End-to-end guard integration: a receiver restart must recover the
    // reserved block from the actual NVS adapter, not merely an in-memory fake.
    Preferences::clearForTest();
    PersistentPeerReplayGuard firstBoot;
    assert(firstBoot.beginAuthenticatedSession(1U, 2U, 100U, 200U, store, 64U));
    PeerCommandMetadata cmd = {2U, 1U, 200U, 1U, 77U, 100U};
    assert(firstBoot.admit(cmd, true) == PEER_COMMAND_ACCEPTED);
    assert(firstBoot.reservedThrough() == 64U);
    firstBoot.endSession();
    PersistentPeerReplayGuard afterRestart;
    assert(afterRestart.beginAuthenticatedSession(1U, 2U, 101U, 200U, store, 64U));
    assert(afterRestart.reservedThrough() == 64U);
    cmd.targetBootId = 101U;
    assert(afterRestart.admit(cmd, true) == PEER_COMMAND_REJECT_REPLAYED_SEQUENCE);
    cmd.senderSequence = 65U; cmd.requestId = 78U;
    assert(afterRestart.admit(cmd, true) == PEER_COMMAND_ACCEPTED);
    assert(afterRestart.reservedThrough() == 128U);

    store.end();
    assert(!store.isOpen());
    puts("PASS: NVS peer replay journal, reboot integration and power-loss fallback tests");
    return 0;
}
