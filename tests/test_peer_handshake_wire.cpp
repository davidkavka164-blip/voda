#include "PeerHandshakeWire.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U;
    hello.responderDeviceId = 2U;
    hello.initiatorBootId = 0x12345678U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);

    uint8_t helloWire[PEER_HANDSHAKE_HELLO_WIRE_SIZE];
    assert(encodePeerHandshakeHello(hello, helloWire));
    assert(helloWire[0] == 'H' && helloWire[1] == 'W' && helloWire[4] == 2U);
    PeerHandshakeHello helloDecoded = {};
    assert(decodePeerHandshakeHello(helloWire, sizeof(helloWire), helloDecoded));
    assert(helloDecoded.initiatorBootId == hello.initiatorBootId);
    assert(memcmp(&helloDecoded, &hello, sizeof(hello)) == 0);

    PeerHandshakeChallenge challenge = {};
    challenge.initiatorDeviceId = 1U;
    challenge.responderDeviceId = 2U;
    challenge.initiatorBootId = 0x12345678U;
    challenge.responderBootId = 0x87654321U;
    challenge.authenticatedReplayFloor = 0x1234U;
    memcpy(challenge.initiatorNonce, hello.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i) {
        challenge.responderNonce[i] = static_cast<uint8_t>(0xA0U + i);
        challenge.tag[i] = static_cast<uint8_t>(0x40U + i);
    }
    uint8_t challengeWire[PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE];
    assert(encodePeerHandshakeChallenge(challenge, challengeWire));
    PeerHandshakeChallenge challengeDecoded = {};
    assert(decodePeerHandshakeChallenge(challengeWire, sizeof(challengeWire), challengeDecoded));
    assert(challengeDecoded.initiatorBootId == challenge.initiatorBootId);
    assert(challengeDecoded.responderBootId == challenge.responderBootId);
    assert(challengeDecoded.authenticatedReplayFloor == challenge.authenticatedReplayFloor);
    assert(memcmp(challengeDecoded.initiatorNonce, challenge.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0);
    assert(memcmp(challengeDecoded.responderNonce, challenge.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0);
    assert(memcmp(challengeDecoded.tag, challenge.tag, PEER_HANDSHAKE_TAG_SIZE) == 0);

    PeerHandshakeFinish finish = {};
    for (size_t i = 0; i < PEER_HANDSHAKE_TAG_SIZE; ++i)
        finish.tag[i] = static_cast<uint8_t>(0xF0U - i);
    uint8_t finishWire[PEER_HANDSHAKE_FINISH_WIRE_SIZE];
    assert(encodePeerHandshakeFinish(finish, finishWire));
    PeerHandshakeFinish finishDecoded = {};
    assert(decodePeerHandshakeFinish(finishWire, sizeof(finishWire), finishDecoded));
    assert(memcmp(finish.tag, finishDecoded.tag, PEER_HANDSHAKE_TAG_SIZE) == 0);

    // Full cryptographic handshake survives canonical wire encode/decode.
    uint8_t secret[32];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(0x31U + i);
    uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(responderNonce); ++i) responderNonce[i] = static_cast<uint8_t>(0x70U + i);
    PeerHandshakeChallenge signedChallenge = {};
    assert(createPeerHandshakeChallenge(hello, 0x87654321U, responderNonce,
                                        secret, sizeof(secret), signedChallenge));
    assert(encodePeerHandshakeChallenge(signedChallenge, challengeWire));
    assert(decodePeerHandshakeChallenge(challengeWire, sizeof(challengeWire), challengeDecoded));
    PeerHandshakeFinish signedFinish = {};
    uint8_t initiatorSession[PEER_HANDSHAKE_TAG_SIZE];
    uint8_t responderSession[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeChallenge(helloDecoded, challengeDecoded, secret, sizeof(secret),
                                        signedFinish, initiatorSession));
    assert(encodePeerHandshakeFinish(signedFinish, finishWire));
    assert(decodePeerHandshakeFinish(finishWire, sizeof(finishWire), finishDecoded));
    assert(verifyPeerHandshakeFinish(helloDecoded, challengeDecoded, finishDecoded,
                                     secret, sizeof(secret), responderSession));
    assert(memcmp(initiatorSession, responderSession, sizeof(initiatorSession)) == 0);

    // Strict lengths, type/version/magic and structural validation.
    assert(!decodePeerHandshakeHello(helloWire, sizeof(helloWire) - 1U, helloDecoded));
    uint8_t bad[PEER_HANDSHAKE_HELLO_WIRE_SIZE];
    memcpy(bad, helloWire, sizeof(bad)); bad[4] = 1U;
    assert(!decodePeerHandshakeHello(bad, sizeof(bad), helloDecoded));
    memcpy(bad, helloWire, sizeof(bad)); bad[0] ^= 1U;
    assert(!decodePeerHandshakeHello(bad, sizeof(bad), helloDecoded));
    memcpy(bad, helloWire, sizeof(bad)); bad[6] = bad[7];
    assert(!decodePeerHandshakeHello(bad, sizeof(bad), helloDecoded));
    assert(!decodePeerHandshakeChallenge(challengeWire, sizeof(challengeWire) - 1U, challengeDecoded));
    uint8_t badChallenge[PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE];
    memcpy(badChallenge, challengeWire, sizeof(badChallenge)); badChallenge[5] = PEER_HANDSHAKE_MESSAGE_HELLO;
    assert(!decodePeerHandshakeChallenge(badChallenge, sizeof(badChallenge), challengeDecoded));
    assert(!decodePeerHandshakeFinish(finishWire, sizeof(finishWire) - 1U, finishDecoded));

    // Failed decodes clear their output, preventing stale values from leaking through.
    memset(&helloDecoded, 0xCC, sizeof(helloDecoded));
    assert(!decodePeerHandshakeHello(0, 0U, helloDecoded));
    const uint8_t* cleared = reinterpret_cast<const uint8_t*>(&helloDecoded);
    for (size_t i = 0; i < sizeof(helloDecoded); ++i) assert(cleared[i] == 0U);

    puts("PASS: PeerHandshakeWire native tests");
    return 0;
}
