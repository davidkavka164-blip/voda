#include "TimedPeerHandshakeResponderCoordinator.h"
#include "PeerHandshakeInitiator.h"
#include <assert.h>
#include <string.h>

using namespace HWC;

class EmptyReplayStore : public IPeerReplayStore {
public:
    PeerReplayStoreLoadResult load(PeerReplayCheckpoint&) { return PEER_REPLAY_STORE_EMPTY; }
    bool saveAtomically(const PeerReplayCheckpoint&) { return true; }
};

static void makeInputs(uint8_t secret[32], uint8_t nonce[PEER_HANDSHAKE_NONCE_SIZE],
                       PeerHandshakeHello& hello) {
    for (size_t i = 0; i < 32U; ++i) secret[i] = static_cast<uint8_t>(i + 9U);
    for (size_t i = 0; i < PEER_HANDSHAKE_NONCE_SIZE; ++i) {
        nonce[i] = static_cast<uint8_t>(0xA0U + i);
        hello.initiatorNonce[i] = static_cast<uint8_t>(0x30U + i);
    }
    hello.initiatorDeviceId = 1U; hello.responderDeviceId = 2U;
    hello.initiatorBootId = 0x12345678UL;
}

int main() {
    uint8_t secret[32], nonce[PEER_HANDSHAKE_NONCE_SIZE];
    PeerHandshakeHello hello = {};
    makeInputs(secret, nonce, hello);
    EmptyReplayStore store;

    // CHALLENGE retries are byte-identical and bounded.
    TimedPeerHandshakeResponderCoordinator exhausted;
    PeerHandshakeChallenge challenge = {}, challengeCopy = {};
    assert(exhausted.begin(100U, 1000U, 50U, 3U, 1U, 2U, 0x87654321UL,
                           hello, nonce, secret, sizeof(secret), store, challenge));
    assert(exhausted.initialChallenge(challengeCopy));
    assert(memcmp(&challenge, &challengeCopy, sizeof(challenge)) == 0);
    assert(!exhausted.challengeRetryDue(149U));
    assert(exhausted.challengeRetryDue(150U));
    assert(exhausted.challengeForRetry(150U, challengeCopy));
    assert(memcmp(&challenge, &challengeCopy, sizeof(challenge)) == 0);
    assert(exhausted.markChallengeSent(150U));
    assert(exhausted.challengeAttempts() == 2U);
    assert(exhausted.challengeRetryDue(200U));
    assert(exhausted.challengeForRetry(200U, challengeCopy));
    assert(memcmp(&challenge, &challengeCopy, sizeof(challenge)) == 0);
    assert(exhausted.markChallengeSent(200U));
    assert(exhausted.poll(250U));
    assert(exhausted.state() == PEER_HS_TIMED_RESP_FAILED);

    // Local UDP send failures consume bounded CHALLENGE retry slots too;
    // they must not leave the responder retrying on every loop iteration.
    TimedPeerHandshakeResponderCoordinator challengeSendFailures;
    assert(challengeSendFailures.begin(500U, 500U, 20U, 3U, 1U, 2U,
                                       0x87654321UL, hello, nonce, secret,
                                       sizeof(secret), store, challenge));
    assert(challengeSendFailures.challengeRetryDue(520U));
    assert(challengeSendFailures.markChallengeAttempted(520U));
    assert(challengeSendFailures.challengeRetryDue(540U));
    assert(challengeSendFailures.markChallengeAttempted(540U));
    assert(challengeSendFailures.poll(560U));
    assert(challengeSendFailures.state() == PEER_HS_TIMED_RESP_FAILED);

    // Full handshake; READY is also retried exactly and the session key is
    // available only after the first READY datagram was accepted for sending.
    TimedPeerHandshakeResponderCoordinator responder;
    assert(responder.begin(0xFFFFFF00UL, 1000U, 40U, 3U, 1U, 2U,
                           0x87654321UL, hello, nonce, secret, sizeof(secret),
                           store, challenge));
    PeerSequenceAllocator allocator;
    assert(allocator.beginBoot(hello.initiatorBootId));
    PeerHandshakeInitiator initiator;
    assert(initiator.begin(hello, secret, sizeof(secret), allocator));
    PeerHandshakeFinish finish = {};
    assert(initiator.acceptChallenge(challenge, finish));
    PeerHandshakeReady ready = {}, readyCopy = {};
    assert(responder.acceptFinish(0xFFFFFF10UL, hello, challenge, finish,
                                  secret, sizeof(secret), ready));
    assert(responder.initialReady(readyCopy));
    assert(memcmp(&ready, &readyCopy, sizeof(ready)) == 0);
    uint8_t key[PEER_HANDSHAKE_TAG_SIZE];
    memset(key, 0xCC, sizeof(key));
    assert(!responder.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    assert(responder.markReadySent(0xFFFFFF10UL));
    assert(responder.copySessionKey(key));
    assert(initiator.acceptReady(ready));
    uint8_t initiatorKey[PEER_HANDSHAKE_TAG_SIZE];
    assert(initiator.copySessionKey(initiatorKey));
    assert(memcmp(key, initiatorKey, sizeof(key)) == 0);

    // Simulate a lost READY: an identical FINISH retrieves the same cached READY.
    assert(responder.acceptFinish(0xFFFFFF20UL, hello, challenge, finish,
                                  secret, sizeof(secret), readyCopy));
    assert(memcmp(&ready, &readyCopy, sizeof(ready)) == 0);
    assert(!responder.readyRetryDue(0xFFFFFF37UL));
    assert(responder.readyRetryDue(0xFFFFFF38UL));
    assert(responder.readyForRetry(0xFFFFFF38UL, readyCopy));
    assert(memcmp(&ready, &readyCopy, sizeof(ready)) == 0);
    assert(responder.markReadyRetrySent(0xFFFFFF38UL));
    assert(responder.readyAttempts() == 2U);
    assert(responder.readyRetryDue(0xFFFFFF60UL));
    assert(responder.readyForRetry(0xFFFFFF60UL, readyCopy));
    assert(responder.markReadyRetrySent(0xFFFFFF60UL));
    assert(!responder.readyRetryDue(0xFFFFFF88UL));
    assert(!responder.poll(0xFFFFFF88UL));
    assert(responder.state() == PEER_HS_TIMED_RESP_ESTABLISHED);
    assert(responder.readyAttempts() == 0U); // exhausted retry policy is retired

    // A modified FINISH is never allowed to retrieve READY.
    PeerHandshakeFinish altered = finish;
    altered.tag[0] ^= 1U;
    assert(!responder.acceptFinish(0xFFFFFF90UL, hello, challenge, altered,
                                   secret, sizeof(secret), readyCopy));
    responder.cancel();
    memset(key, 0xCC, sizeof(key));
    assert(!responder.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    // The overall handshake deadline also applies after FINISH while READY
    // has not yet been accepted by the transport.
    TimedPeerHandshakeResponderCoordinator pendingReady;
    assert(pendingReady.begin(1000U, 100U, 20U, 3U, 1U, 2U, 0x87654321UL,
                              hello, nonce, secret, sizeof(secret), store, challenge));
    PeerSequenceAllocator secondAllocator;
    assert(secondAllocator.beginBoot(hello.initiatorBootId));
    PeerHandshakeInitiator secondInitiator;
    assert(secondInitiator.begin(hello, secret, sizeof(secret), secondAllocator));
    assert(secondInitiator.acceptChallenge(challenge, finish));
    assert(pendingReady.acceptFinish(1050U, hello, challenge, finish,
                                     secret, sizeof(secret), ready));
    assert(pendingReady.state() == PEER_HS_TIMED_RESP_READY_PENDING);
    assert(pendingReady.poll(1100U));
    assert(pendingReady.state() == PEER_HS_TIMED_RESP_FAILED);
    memset(key, 0xCC, sizeof(key));
    assert(!pendingReady.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    // A failed initial READY send must not disable retries. The first
    // successful retransmission is allowed to establish the responder session.
    TimedPeerHandshakeResponderCoordinator lostInitialReady;
    assert(lostInitialReady.begin(2000U, 500U, 20U, 3U, 1U, 2U,
                                  0x87654321UL, hello, nonce, secret, sizeof(secret),
                                  store, challenge));
    PeerSequenceAllocator thirdAllocator;
    assert(thirdAllocator.beginBoot(hello.initiatorBootId));
    PeerHandshakeInitiator thirdInitiator;
    assert(thirdInitiator.begin(hello, secret, sizeof(secret), thirdAllocator));
    assert(thirdInitiator.acceptChallenge(challenge, finish));
    assert(lostInitialReady.acceptFinish(2010U, hello, challenge, finish,
                                         secret, sizeof(secret), ready));
    memset(key, 0xCC, sizeof(key));
    assert(!lostInitialReady.copySessionKey(key));
    assert(!lostInitialReady.readyRetryDue(2029U));
    assert(lostInitialReady.readyRetryDue(2030U));
    assert(lostInitialReady.readyForRetry(2030U, readyCopy));
    assert(memcmp(&ready, &readyCopy, sizeof(ready)) == 0);
    assert(lostInitialReady.markReadyRetrySent(2030U));
    assert(lostInitialReady.state() == PEER_HS_TIMED_RESP_ESTABLISHED);
    assert(lostInitialReady.copySessionKey(key));

    // If every READY attempt fails, the responder fails closed and erases
    // the session key instead of remaining indefinitely pending.
    TimedPeerHandshakeResponderCoordinator allReadyLost;
    assert(allReadyLost.begin(3000U, 500U, 20U, 3U, 1U, 2U,
                              0x87654321UL, hello, nonce, secret, sizeof(secret),
                              store, challenge));
    PeerSequenceAllocator fourthAllocator;
    assert(fourthAllocator.beginBoot(hello.initiatorBootId));
    PeerHandshakeInitiator fourthInitiator;
    assert(fourthInitiator.begin(hello, secret, sizeof(secret), fourthAllocator));
    assert(fourthInitiator.acceptChallenge(challenge, finish));
    assert(allReadyLost.acceptFinish(3010U, hello, challenge, finish,
                                     secret, sizeof(secret), ready));
    assert(allReadyLost.readyRetryDue(3030U));
    assert(allReadyLost.markReadyRetryAttempted(3030U, false));
    assert(allReadyLost.readyRetryDue(3050U));
    assert(allReadyLost.markReadyRetryAttempted(3050U, false));
    assert(allReadyLost.poll(3070U));
    assert(allReadyLost.state() == PEER_HS_TIMED_RESP_FAILED);
    memset(key, 0xCC, sizeof(key));
    assert(!allReadyLost.copySessionKey(key));
    for (size_t i = 0; i < sizeof(key); ++i) assert(key[i] == 0U);

    // Invalid retry parameters must fail closed before a challenge is exposed.
    TimedPeerHandshakeResponderCoordinator invalid;
    assert(!invalid.begin(0U, 1000U, 0U, 3U, 1U, 2U, 0x87654321UL,
                          hello, nonce, secret, sizeof(secret), store, challenge));
    assert(invalid.state() == PEER_HS_TIMED_RESP_FAILED);
    assert(!invalid.initialChallenge(challenge));
    return 0;
}
