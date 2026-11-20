#include "PeerHandshakeInitiator.h"
#include "PeerHandshakeReady.h"
#include <assert.h>
#include <string.h>

using namespace HWC;

static PeerHandshakeHello makeHello() {
    PeerHandshakeHello h = {};
    h.initiatorDeviceId = 1U;
    h.responderDeviceId = 2U;
    h.initiatorBootId = 1001U;
    for (size_t i = 0U; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        h.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);
    return h;
}

int main() {
    uint8_t secret[32];
    for (size_t i = 0U; i < sizeof(secret); ++i)
        secret[i] = static_cast<uint8_t>(0xA0U + i);

    PeerSequenceAllocator allocator;
    assert(allocator.beginBoot(1001U));
    PeerHandshakeInitiator initiator;
    PeerHandshakeHello hello = makeHello();
    assert(initiator.begin(hello, secret, sizeof(secret), allocator));
    assert(initiator.state() == PEER_HS_INIT_WAIT_CHALLENGE);

    uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0U; i < sizeof(responderNonce); ++i)
        responderNonce[i] = static_cast<uint8_t>(0x40U + i);
    PeerHandshakeChallenge challenge;
    assert(createPeerHandshakeChallenge(hello, 2002U, responderNonce,
                                        secret, sizeof(secret), challenge, 64U));

    PeerHandshakeChallenge tampered = challenge;
    tampered.authenticatedReplayFloor = 128U;
    PeerHandshakeFinish finish;
    assert(!initiator.acceptChallenge(tampered, finish));
    assert(initiator.state() == PEER_HS_INIT_FAILED);
    assert(allocator.nextSequence() == 1U);

    assert(initiator.begin(hello, secret, sizeof(secret), allocator));
    assert(initiator.acceptChallenge(challenge, finish));
    assert(initiator.state() == PEER_HS_INIT_WAIT_READY);
    assert(allocator.nextSequence() == 65U);
    assert(initiator.authenticatedReplayFloor() == 64U);

    uint8_t keyBeforeReady[PEER_HANDSHAKE_TAG_SIZE];
    memset(keyBeforeReady, 0xCC, sizeof(keyBeforeReady));
    assert(!initiator.copySessionKey(keyBeforeReady));
    for (size_t i = 0U; i < sizeof(keyBeforeReady); ++i)
        assert(keyBeforeReady[i] == 0U);

    PeerHandshakeFinish retry;
    assert(initiator.finishForRetry(retry));
    assert(memcmp(&retry, &finish, sizeof(finish)) == 0);

    uint8_t responderKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeFinish(hello, challenge, finish, secret,
                                     sizeof(secret), responderKey));
    PeerHandshakeReady ready;
    assert(createPeerHandshakeReady(responderKey, sizeof(responderKey), ready));
    PeerHandshakeReady altered = ready;
    altered.tag[0] ^= 0x01U;
    assert(!initiator.acceptReady(altered));
    assert(!initiator.established());
    assert(!initiator.copySessionKey(keyBeforeReady));

    assert(initiator.begin(hello, secret, sizeof(secret), allocator));
    assert(initiator.acceptChallenge(challenge, finish));
    assert(initiator.acceptReady(ready));
    assert(initiator.established());
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(initiator.copySessionKey(initiatorKey));
    assert(memcmp(initiatorKey, responderKey, sizeof(initiatorKey)) == 0);

    initiator.cancel();
    memset(initiatorKey, 0xCC, sizeof(initiatorKey));
    assert(!initiator.copySessionKey(initiatorKey));
    for (size_t i = 0U; i < sizeof(initiatorKey); ++i)
        assert(initiatorKey[i] == 0U);
    return 0;
}
