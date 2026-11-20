#include "TimedPeerHandshakeInitiator.h"
#include "PeerHandshakeResponderCoordinator.h"
#include <assert.h>
#include <string.h>

using namespace HWC;

class EmptyReplayStore : public IPeerReplayStore {
public:
    PeerReplayStoreLoadResult load(PeerReplayCheckpoint&) { return PEER_REPLAY_STORE_EMPTY; }
    bool saveAtomically(const PeerReplayCheckpoint&) { return true; }
};

int main() {
    uint8_t secret[32], responderNonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 9U);
    for (size_t i = 0; i < sizeof(responderNonce); ++i) responderNonce[i] = static_cast<uint8_t>(0xA0U + i);

    PeerSequenceAllocator allocator;
    assert(allocator.beginBoot(101U));
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U; hello.responderDeviceId = 2U;
    hello.initiatorBootId = 101U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);

    TimedPeerHandshakeInitiator initiator;
    assert(initiator.begin(0xFFFFFF00UL, 2000U, 100U, 3U,
                           hello, secret, sizeof(secret), allocator));
    assert(initiator.state() == PEER_HS_TIMED_INIT_HELLO_PENDING);
    PeerHandshakeHello initialHello;
    assert(initiator.initialHello(initialHello));
    assert(memcmp(&initialHello, &hello, sizeof(hello)) == 0);
    assert(!initiator.helloRetryDue(0xFFFFFF63UL));
    assert(initiator.helloRetryDue(0xFFFFFF64UL));
    PeerHandshakeHello retryHello;
    assert(initiator.helloForRetry(0xFFFFFF64UL, retryHello));
    assert(memcmp(&retryHello, &initialHello, sizeof(hello)) == 0);
    assert(initiator.markHelloSent(0xFFFFFF64UL));
    assert(initiator.attempts() == 2U);

    EmptyReplayStore store;
    PeerHandshakeResponderCoordinator responder;
    PeerHandshakeChallenge challenge;
    assert(responder.begin(0xFFFFFF65UL, 1500U, 1U, 2U, 202U,
                           hello, responderNonce, secret, sizeof(secret), store, challenge));
    PeerHandshakeFinish finish;
    assert(initiator.acceptChallenge(0xFFFFFF70UL, challenge, finish));
    assert(initiator.state() == PEER_HS_TIMED_INIT_FINISH_PENDING);
    uint8_t prematureKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(!initiator.copySessionKey(prematureKey));
    for (size_t i = 0; i < sizeof(prematureKey); ++i) assert(prematureKey[i] == 0U);

    PeerHandshakeReady ready;
    assert(responder.acceptFinish(0xFFFFFF71UL, hello, challenge, finish,
                                  secret, sizeof(secret), ready));
    assert(initiator.finishRetryDue(0x00000069UL)); // wrap-safe 249 ms elapsed
    PeerHandshakeFinish finishRetry;
    assert(initiator.finishForRetry(0x00000069UL, finishRetry));
    assert(memcmp(&finishRetry, &finish, sizeof(finish)) == 0);
    assert(initiator.markFinishSent(0x00000069UL));
    assert(initiator.attempts() == 2U);

    assert(initiator.acceptReady(0x00000071UL, ready));
    assert(initiator.state() == PEER_HS_TIMED_INIT_ESTABLISHED);
    uint8_t key[PEER_HANDSHAKE_TAG_SIZE];
    assert(initiator.copySessionKey(key));
    assert(memcmp(key, secret, sizeof(key)) != 0);
    assert(!initiator.poll(0x00001000UL));

    // Exhausted retries fail closed and erase the pending key material.
    PeerSequenceAllocator allocator2;
    assert(allocator2.beginBoot(303U));
    hello.initiatorBootId = 303U;
    TimedPeerHandshakeInitiator exhausted;
    assert(exhausted.begin(100U, 1000U, 100U, 2U, hello,
                           secret, sizeof(secret), allocator2));
    assert(!exhausted.poll(199U));
    assert(exhausted.helloRetryDue(200U));
    PeerHandshakeHello finalHello;
    assert(exhausted.helloForRetry(200U, finalHello));
    assert(exhausted.markHelloSent(200U));
    assert(exhausted.poll(299U) == false);
    assert(exhausted.poll(300U));
    assert(exhausted.state() == PEER_HS_TIMED_INIT_FAILED);
    memset(key, 0xCC, sizeof(key));
    assert(!exhausted.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    // An invalid READY terminates the initiator immediately and clears key material.
    PeerSequenceAllocator allocatorBad;
    assert(allocatorBad.beginBoot(350U));
    hello.initiatorBootId = 350U;
    TimedPeerHandshakeInitiator badReadyInitiator;
    assert(badReadyInitiator.begin(400U, 1000U, 100U, 3U, hello,
                                   secret, sizeof(secret), allocatorBad));
    PeerHandshakeHello badHello;
    assert(badReadyInitiator.initialHello(badHello));
    PeerHandshakeChallenge badChallenge;
    assert(responder.begin(401U, 1000U, 1U, 2U, 202U, badHello,
                           responderNonce, secret, sizeof(secret), store, badChallenge));
    PeerHandshakeFinish badFinish;
    assert(badReadyInitiator.acceptChallenge(402U, badChallenge, badFinish));
    PeerHandshakeReady invalidReady = {};
    assert(!badReadyInitiator.acceptReady(403U, invalidReady));
    assert(badReadyInitiator.state() == PEER_HS_TIMED_INIT_FAILED);
    memset(key, 0xCC, sizeof(key));
    assert(!badReadyInitiator.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    // Overall deadline also wins over a not-yet-due retry interval.
    PeerSequenceAllocator allocator3;
    assert(allocator3.beginBoot(404U));
    hello.initiatorBootId = 404U;
    TimedPeerHandshakeInitiator deadline;
    assert(deadline.begin(1000U, 50U, 500U, 3U, hello,
                          secret, sizeof(secret), allocator3));
    assert(deadline.poll(1050U));
    assert(deadline.state() == PEER_HS_TIMED_INIT_FAILED);
    return 0;
}
