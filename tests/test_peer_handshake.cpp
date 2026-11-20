#include "PeerHandshake.h"
#include "PeerSequenceAllocator.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    uint8_t secret[32];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 1U);

    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U;
    hello.responderDeviceId = 2U;
    hello.initiatorBootId = 0x10203040U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(0x20U + i);

    uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(responderNonce); ++i)
        responderNonce[i] = static_cast<uint8_t>(0xA0U + i);

    PeerHandshakeChallenge challenge;
    assert(createPeerHandshakeChallenge(hello, 0x50607080U, responderNonce,
                                        secret, sizeof(secret), challenge, 128U));
    assert(challenge.authenticatedReplayFloor == 128U);
    PeerHandshakeFinish finish;
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    uint8_t responderKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                        finish, initiatorKey));
    assert(verifyPeerHandshakeFinish(hello, challenge, finish, secret, sizeof(secret),
                                     responderKey));
    assert(memcmp(initiatorKey, responderKey, sizeof(initiatorKey)) == 0);
    // The replay floor is authenticated as part of the transcript and can be
    // applied by the initiator only after successful CHALLENGE verification.
    PeerSequenceAllocator synchronizedSender;
    assert(synchronizedSender.beginBoot(hello.initiatorBootId));
    assert(synchronizedSender.advancePastAuthenticatedFloor(challenge.authenticatedReplayFloor));
    uint32_t firstAfterSync = 0U;
    assert(synchronizedSender.allocate(firstAfterSync));
    assert(firstAfterSync == 129U);

    // Wrong secret, modified challenge and modified FINISH are rejected.
    uint8_t wrongSecret[32];
    memcpy(wrongSecret, secret, sizeof(secret));
    wrongSecret[0] ^= 0x80U;
    PeerHandshakeFinish rejectedFinish;
    uint8_t rejectedKey[PEER_HANDSHAKE_TAG_SIZE];
    memset(rejectedKey, 0xCC, sizeof(rejectedKey));
    assert(!verifyPeerHandshakeChallenge(hello, challenge, wrongSecret,
                                         sizeof(wrongSecret), rejectedFinish, rejectedKey));
    for (size_t i = 0; i < sizeof(rejectedKey); ++i) assert(rejectedKey[i] == 0U);

    PeerHandshakeChallenge tamperedChallenge = challenge;
    tamperedChallenge.authenticatedReplayFloor++;
    assert(!verifyPeerHandshakeChallenge(hello, tamperedChallenge, secret,
                                         sizeof(secret), rejectedFinish, rejectedKey));
    tamperedChallenge = challenge;
    tamperedChallenge.responderBootId++;
    assert(!verifyPeerHandshakeChallenge(hello, tamperedChallenge, secret,
                                         sizeof(secret), rejectedFinish, rejectedKey));
    PeerHandshakeFinish tamperedFinish = finish;
    tamperedFinish.tag[7] ^= 1U;
    assert(!verifyPeerHandshakeFinish(hello, challenge, tamperedFinish, secret,
                                      sizeof(secret), rejectedKey));

    // Refuse invalid IDs, zero/reused nonce, zero boot IDs and weak secrets.
    PeerHandshakeHello badHello = hello;
    badHello.responderDeviceId = badHello.initiatorDeviceId;
    assert(!createPeerHandshakeChallenge(badHello, 3U, responderNonce,
                                         secret, sizeof(secret), challenge));
    badHello = hello;
    memset(badHello.initiatorNonce, 0, sizeof(badHello.initiatorNonce));
    assert(!createPeerHandshakeChallenge(badHello, 3U, responderNonce,
                                         secret, sizeof(secret), challenge));
    assert(!createPeerHandshakeChallenge(hello, 3U, hello.initiatorNonce,
                                         secret, sizeof(secret), challenge));
    assert(!createPeerHandshakeChallenge(hello, 0U, responderNonce,
                                         secret, sizeof(secret), challenge));
    assert(!createPeerHandshakeChallenge(hello, 3U, responderNonce,
                                         secret, 16U, challenge));

    // Stateful responder pins the peer identity and consumes a FINISH once.
    PeerHandshakeResponder responder;
    PeerHandshakeChallenge statefulChallenge;
    assert(responder.begin(1U, 2U, 0x50607080U, hello, responderNonce,
                           secret, sizeof(secret), statefulChallenge));
    assert(responder.pending());
    assert(!responder.begin(1U, 2U, 0x50607080U, hello, responderNonce,
                            secret, sizeof(secret), statefulChallenge));
    PeerHandshakeFinish statefulFinish;
    uint8_t statefulInitiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    uint8_t statefulResponderKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeChallenge(hello, statefulChallenge, secret,
                                        sizeof(secret), statefulFinish, statefulInitiatorKey));
    assert(responder.acceptFinish(hello, statefulChallenge, statefulFinish, secret,
                                  sizeof(secret), statefulResponderKey));
    assert(!responder.pending());
    assert(memcmp(statefulInitiatorKey, statefulResponderKey, sizeof(statefulInitiatorKey)) == 0);
    memset(rejectedKey, 0xCC, sizeof(rejectedKey));
    assert(!responder.acceptFinish(hello, statefulChallenge, statefulFinish, secret,
                                   sizeof(secret), rejectedKey));
    for (size_t i = 0; i < sizeof(rejectedKey); ++i) assert(rejectedKey[i] == 0U);

    PeerHandshakeResponder wrongPeerResponder;
    assert(!wrongPeerResponder.begin(3U, 2U, 0x50607080U, hello, responderNonce,
                                     secret, sizeof(secret), statefulChallenge));

    // Transcript binds both device identities and both boot IDs.
    assert(createPeerHandshakeChallenge(hello, 0x50607080U, responderNonce,
                                        secret, sizeof(secret), challenge));
    PeerHandshakeHello changedHello = hello;
    changedHello.initiatorBootId++;
    assert(!verifyPeerHandshakeChallenge(changedHello, challenge, secret,
                                         sizeof(secret), rejectedFinish, rejectedKey));
    puts("PASS: PeerHandshake native tests");
    return 0;
}
