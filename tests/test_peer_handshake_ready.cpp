#include "PeerHandshake.h"
#include "PeerHandshakeReady.h"
#include "PeerHandshakeWire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    uint8_t secret[32];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 3U);
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U;
    hello.responderDeviceId = 2U;
    hello.initiatorBootId = 0x12345678U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);
    uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(responderNonce); ++i)
        responderNonce[i] = static_cast<uint8_t>(0xA0U + i);

    PeerHandshakeChallenge challenge = {};
    assert(createPeerHandshakeChallenge(hello, 0x87654321U, responderNonce,
                                        secret, sizeof(secret), challenge, 64U));
    PeerHandshakeFinish finish = {};
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE] = {};
    uint8_t responderKey[PEER_HANDSHAKE_TAG_SIZE] = {};
    assert(verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                        finish, initiatorKey));
    assert(verifyPeerHandshakeFinish(hello, challenge, finish, secret, sizeof(secret),
                                     responderKey));
    assert(memcmp(initiatorKey, responderKey, sizeof(initiatorKey)) == 0);

    // Responder acceptance is not enough: initiator waits for READY.
    PeerHandshakeReady ready = {};
    assert(createPeerHandshakeReady(responderKey, sizeof(responderKey), ready));
    uint8_t wire[PEER_HANDSHAKE_READY_WIRE_SIZE];
    assert(encodePeerHandshakeReady(ready, wire));
    assert(wire[4] == 2U && wire[5] == PEER_HANDSHAKE_MESSAGE_READY);
    PeerHandshakeReady decoded = {};
    assert(decodePeerHandshakeReady(wire, sizeof(wire), decoded));
    assert(verifyPeerHandshakeReady(initiatorKey, sizeof(initiatorKey), decoded));

    PeerHandshakeReady tampered = decoded;
    tampered.tag[0] ^= 0x01U;
    assert(!verifyPeerHandshakeReady(initiatorKey, sizeof(initiatorKey), tampered));
    uint8_t wrongKey[PEER_HANDSHAKE_TAG_SIZE];
    memcpy(wrongKey, initiatorKey, sizeof(wrongKey));
    wrongKey[31] ^= 0x80U;
    assert(!verifyPeerHandshakeReady(wrongKey, sizeof(wrongKey), decoded));
    assert(!verifyPeerHandshakeReady(initiatorKey, 16U, decoded));
    assert(!decodePeerHandshakeReady(wire, sizeof(wire) - 1U, decoded));
    uint8_t badWire[PEER_HANDSHAKE_READY_WIRE_SIZE];
    memcpy(badWire, wire, sizeof(badWire));
    badWire[5] = PEER_HANDSHAKE_MESSAGE_FINISH;
    assert(!decodePeerHandshakeReady(badWire, sizeof(badWire), decoded));

    puts("PASS: PeerHandshakeReady native tests");
    return 0;
}
