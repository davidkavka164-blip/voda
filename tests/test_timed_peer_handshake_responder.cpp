#include "TimedPeerHandshakeResponder.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    uint8_t secret[32], peerNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 1U);
    for (size_t i = 0; i < sizeof(peerNonce); ++i) peerNonce[i] = static_cast<uint8_t>(0xA0U + i);
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U;
    hello.responderDeviceId = 2U;
    hello.initiatorBootId = 123U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(0x20U + i);

    TimedPeerHandshakeResponder timed;
    PeerHandshakeChallenge challenge = {};
    assert(!timed.begin(100U, 0U, 1U, 2U, 456U, hello, peerNonce,
                        secret, sizeof(secret), challenge));
    assert(!timed.pending());

    assert(timed.begin(100U, 5000U, 1U, 2U, 456U, hello, peerNonce,
                       secret, sizeof(secret), challenge));
    assert(timed.pending());
    assert(!timed.poll(5099U));
    assert(timed.pending());
    assert(timed.poll(5100U));
    assert(!timed.pending());

    // A valid FINISH arriving exactly at the deadline is rejected and key cleared.
    assert(timed.begin(1000U, 500U, 1U, 2U, 456U, hello, peerNonce,
                       secret, sizeof(secret), challenge));
    PeerHandshakeFinish finish = {};
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    uint8_t responderKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                        finish, initiatorKey));
    memset(responderKey, 0xCC, sizeof(responderKey));
    assert(!timed.acceptFinish(1500U, hello, challenge, finish, secret,
                               sizeof(secret), responderKey));
    assert(!timed.pending());
    for (size_t i = 0; i < sizeof(responderKey); ++i) assert(responderKey[i] == 0U);

    // A valid FINISH before deadline succeeds once.
    assert(timed.begin(0xFFFFFF00UL, 512U, 1U, 2U, 456U, hello, peerNonce,
                       secret, sizeof(secret), challenge));
    assert(verifyPeerHandshakeChallenge(hello, challenge, secret, sizeof(secret),
                                        finish, initiatorKey));
    assert(timed.acceptFinish(0x000000FFUL, hello, challenge, finish, secret,
                              sizeof(secret), responderKey));
    assert(memcmp(initiatorKey, responderKey, sizeof(initiatorKey)) == 0);
    assert(!timed.pending());
    assert(!timed.acceptFinish(0x00000100UL, hello, challenge, finish, secret,
                               sizeof(secret), responderKey));

    puts("PASS: TimedPeerHandshakeResponder deadline, wrap and one-shot tests");
    return 0;
}
