#include "PeerHandshakeResponderCoordinator.h"
#include "PeerHandshakeInitiator.h"
#include <assert.h>
#include <string.h>

using namespace HWC;

class EmptyReplayStore : public IPeerReplayStore {
public:
    PeerReplayStoreLoadResult load(PeerReplayCheckpoint&) { return PEER_REPLAY_STORE_EMPTY; }
    bool saveAtomically(const PeerReplayCheckpoint&) { return true; }
};

int main() {
    uint8_t secret[32], nonce[PEER_HANDSHAKE_NONCE_SIZE];
    for (size_t i = 0; i < sizeof(secret); ++i) secret[i] = static_cast<uint8_t>(i + 3U);
    for (size_t i = 0; i < sizeof(nonce); ++i) nonce[i] = static_cast<uint8_t>(0x70U + i);
    PeerHandshakeHello hello = {};
    hello.initiatorDeviceId = 1U; hello.responderDeviceId = 2U;
    hello.initiatorBootId = 101U;
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i)
        hello.initiatorNonce[i] = static_cast<uint8_t>(i + 1U);

    EmptyReplayStore store;
    PeerHandshakeResponderCoordinator responder;
    PeerHandshakeChallenge challenge;
    assert(responder.begin(100U, 1000U, 1U, 2U, 202U, hello, nonce,
                           secret, sizeof(secret), store, challenge));
    assert(responder.state() == PEER_HS_RESP_WAIT_FINISH);

    PeerSequenceAllocator allocator;
    assert(allocator.beginBoot(101U));
    PeerHandshakeInitiator initiator;
    assert(initiator.begin(hello, secret, sizeof(secret), allocator));
    PeerHandshakeFinish finish;
    assert(initiator.acceptChallenge(challenge, finish));

    PeerHandshakeReady ready;
    assert(responder.acceptFinish(200U, hello, challenge, finish,
                                  secret, sizeof(secret), ready));
    assert(responder.state() == PEER_HS_RESP_READY_PENDING);
    // Lost READY: an identical FINISH must retrieve the byte-identical READY.
    PeerHandshakeReady duplicateReady;
    assert(responder.acceptFinish(210U, hello, challenge, finish,
                                  secret, sizeof(secret), duplicateReady));
    assert(memcmp(&duplicateReady, &ready, sizeof(ready)) == 0);
    // A modified FINISH must never trigger READY retransmission.
    PeerHandshakeFinish alteredFinish = finish;
    alteredFinish.tag[0] ^= 0x80U;
    assert(!responder.acceptFinish(220U, hello, challenge, alteredFinish,
                                   secret, sizeof(secret), duplicateReady));
    uint8_t responderKey[PEER_HANDSHAKE_TAG_SIZE];
    memset(responderKey, 0xCC, sizeof(responderKey));
    assert(!responder.copySessionKey(responderKey));
    for (size_t i = 0; i < sizeof(responderKey); ++i) assert(responderKey[i] == 0U);

    PeerHandshakeReady retry;
    assert(responder.readyForRetry(retry));
    assert(memcmp(&retry, &ready, sizeof(ready)) == 0);
    assert(initiator.acceptReady(retry));
    assert(responder.markReadyTransmitted(250U));
    assert(responder.copySessionKey(responderKey));
    // A late duplicate after the local send was accepted also gets cached READY.
    assert(responder.acceptFinish(5000U, hello, challenge, finish,
                                  secret, sizeof(secret), duplicateReady));
    assert(memcmp(&duplicateReady, &ready, sizeof(ready)) == 0);
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(initiator.copySessionKey(initiatorKey));
    assert(memcmp(initiatorKey, responderKey, sizeof(initiatorKey)) == 0);

    responder.cancel();
    memset(responderKey, 0xCC, sizeof(responderKey));
    assert(!responder.copySessionKey(responderKey));
    for (size_t i = 0; i < sizeof(responderKey); ++i) assert(responderKey[i] == 0U);

    // Deadline applies while waiting for FINISH and while READY is pending.
    assert(responder.begin(1000U, 100U, 1U, 2U, 202U, hello, nonce,
                           secret, sizeof(secret), store, challenge));
    assert(responder.poll(1100U));
    assert(responder.state() == PEER_HS_RESP_FAILED);

    // The same deadline also covers READY_PENDING; expired duplicates must
    // not retrieve the cached proof or leave a usable session key behind.
    assert(responder.begin(2000U, 100U, 1U, 2U, 202U, hello, nonce,
                           secret, sizeof(secret), store, challenge));
    assert(responder.acceptFinish(2050U, hello, challenge, finish,
                                  secret, sizeof(secret), ready));
    assert(responder.state() == PEER_HS_RESP_READY_PENDING);
    assert(!responder.acceptFinish(2100U, hello, challenge, finish,
                                  secret, sizeof(secret), duplicateReady));
    assert(responder.state() == PEER_HS_RESP_FAILED);
    memset(responderKey, 0xCC, sizeof(responderKey));
    assert(!responder.copySessionKey(responderKey));
    for (size_t i = 0; i < sizeof(responderKey); ++i) assert(responderKey[i] == 0U);
    return 0;
}
