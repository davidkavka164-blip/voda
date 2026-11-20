#include "PeerHandshake.h"
#include "HmacSha256.h"
#include "PersistentPeerReplayGuard.h"
#include <string.h>

namespace HWC { namespace {
static const uint8_t CHALLENGE_DOMAIN[] = "HWC-HS-CHALLENGE-V1";
static const uint8_t FINISH_DOMAIN[] = "HWC-HS-FINISH-V1";
static const uint8_t SESSION_DOMAIN[] = "HWC-SESSION-V1";
static const size_t TRANSCRIPT_SIZE = 47U;

void put32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8U);
    p[2] = static_cast<uint8_t>(v >> 16U);
    p[3] = static_cast<uint8_t>(v >> 24U);
}

bool nonzero(const uint8_t* p, size_t n) {
    uint8_t v = 0U;
    for (size_t i = 0; i < n; ++i) v |= p[i];
    return v != 0U;
}

bool validHello(const PeerHandshakeHello& h) {
    return h.initiatorDeviceId != 0U && h.responderDeviceId != 0U &&
           h.initiatorDeviceId != h.responderDeviceId && h.initiatorBootId != 0U &&
           nonzero(h.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
}

bool validChallenge(const PeerHandshakeHello& h, const PeerHandshakeChallenge& c) {
    return validHello(h) && c.initiatorDeviceId == h.initiatorDeviceId &&
           c.responderDeviceId == h.responderDeviceId &&
           c.initiatorBootId == h.initiatorBootId && c.responderBootId != 0U &&
           c.authenticatedReplayFloor < 0x80000000UL &&
           memcmp(c.initiatorNonce, h.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0 &&
           nonzero(c.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) &&
           memcmp(c.initiatorNonce, c.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) != 0;
}

void makeTranscript(const PeerHandshakeHello& h, uint32_t responderBootId,
                    const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
                    uint32_t authenticatedReplayFloor,
                    uint8_t out[TRANSCRIPT_SIZE]) {
    out[0] = 1U;
    out[1] = h.initiatorDeviceId;
    out[2] = h.responderDeviceId;
    put32(out + 3U, h.initiatorBootId);
    put32(out + 7U, responderBootId);
    put32(out + 11U, authenticatedReplayFloor);
    memcpy(out + 15U, h.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(out + 31U, responderNonce, PEER_HANDSHAKE_NONCE_SIZE);
}

void macDomain(const uint8_t* secret, size_t secretLength,
               const uint8_t* domain, size_t domainLength,
               const uint8_t transcript[TRANSCRIPT_SIZE], uint8_t out[HMAC_SHA256_SIZE]) {
    uint8_t message[32U + TRANSCRIPT_SIZE];
    memcpy(message, domain, domainLength);
    memcpy(message + domainLength, transcript, TRANSCRIPT_SIZE);
    hmacSha256(secret, secretLength, message, domainLength + TRANSCRIPT_SIZE, out);
    memset(message, 0, sizeof(message));
}

bool validSecret(const uint8_t* secret, size_t length) {
    return secret != 0 && length >= PEER_HANDSHAKE_MIN_SECRET_SIZE;
}

void clearOutputs(PeerHandshakeFinish* finish, uint8_t* key) {
    if (finish) memset(finish, 0, sizeof(*finish));
    if (key) memset(key, 0, PEER_HANDSHAKE_TAG_SIZE);
}
} // namespace

bool createPeerHandshakeChallenge(const PeerHandshakeHello& hello,
                                  uint32_t responderBootId,
                                  const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
                                  const uint8_t* secret, size_t secretLength,
                                  PeerHandshakeChallenge& challenge,
                                  uint32_t authenticatedReplayFloor) {
    memset(&challenge, 0, sizeof(challenge));
    if (!validHello(hello) || responderBootId == 0U || responderNonce == 0 ||
        !nonzero(responderNonce, PEER_HANDSHAKE_NONCE_SIZE) ||
        memcmp(hello.initiatorNonce, responderNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0 ||
        authenticatedReplayFloor >= 0x80000000UL ||
        !validSecret(secret, secretLength)) return false;
    challenge.initiatorDeviceId = hello.initiatorDeviceId;
    challenge.responderDeviceId = hello.responderDeviceId;
    challenge.initiatorBootId = hello.initiatorBootId;
    challenge.responderBootId = responderBootId;
    challenge.authenticatedReplayFloor = authenticatedReplayFloor;
    memcpy(challenge.initiatorNonce, hello.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(challenge.responderNonce, responderNonce, PEER_HANDSHAKE_NONCE_SIZE);
    uint8_t transcript[TRANSCRIPT_SIZE];
    makeTranscript(hello, responderBootId, responderNonce, authenticatedReplayFloor, transcript);
    macDomain(secret, secretLength, CHALLENGE_DOMAIN, sizeof(CHALLENGE_DOMAIN) - 1U,
              transcript, challenge.tag);
    memset(transcript, 0, sizeof(transcript));
    return true;
}

bool verifyPeerHandshakeChallenge(const PeerHandshakeHello& hello,
                                   const PeerHandshakeChallenge& challenge,
                                   const uint8_t* secret, size_t secretLength,
                                   PeerHandshakeFinish& finish,
                                   uint8_t sessionKey[PEER_HANDSHAKE_TAG_SIZE]) {
    clearOutputs(&finish, sessionKey);
    if (!validSecret(secret, secretLength) || sessionKey == 0 ||
        !validChallenge(hello, challenge)) return false;
    uint8_t transcript[TRANSCRIPT_SIZE];
    uint8_t expected[HMAC_SHA256_SIZE];
    makeTranscript(hello, challenge.responderBootId, challenge.responderNonce,
                   challenge.authenticatedReplayFloor, transcript);
    macDomain(secret, secretLength, CHALLENGE_DOMAIN, sizeof(CHALLENGE_DOMAIN) - 1U,
              transcript, expected);
    const bool authenticated = constantTimeTagEquals(expected, challenge.tag);
    memset(expected, 0, sizeof(expected));
    if (!authenticated) {
        memset(transcript, 0, sizeof(transcript));
        return false;
    }
    macDomain(secret, secretLength, FINISH_DOMAIN, sizeof(FINISH_DOMAIN) - 1U,
              transcript, finish.tag);
    macDomain(secret, secretLength, SESSION_DOMAIN, sizeof(SESSION_DOMAIN) - 1U,
              transcript, sessionKey);
    memset(transcript, 0, sizeof(transcript));
    return true;
}

bool verifyPeerHandshakeFinish(const PeerHandshakeHello& hello,
                               const PeerHandshakeChallenge& challenge,
                               const PeerHandshakeFinish& finish,
                               const uint8_t* secret, size_t secretLength,
                               uint8_t sessionKey[PEER_HANDSHAKE_TAG_SIZE]) {
    if (sessionKey) memset(sessionKey, 0, PEER_HANDSHAKE_TAG_SIZE);
    if (!validSecret(secret, secretLength) || sessionKey == 0 ||
        !validChallenge(hello, challenge)) return false;
    uint8_t transcript[TRANSCRIPT_SIZE];
    uint8_t expectedChallenge[HMAC_SHA256_SIZE];
    uint8_t expectedFinish[HMAC_SHA256_SIZE];
    makeTranscript(hello, challenge.responderBootId, challenge.responderNonce,
                   challenge.authenticatedReplayFloor, transcript);
    macDomain(secret, secretLength, CHALLENGE_DOMAIN, sizeof(CHALLENGE_DOMAIN) - 1U,
              transcript, expectedChallenge);
    const bool challengeValid = constantTimeTagEquals(expectedChallenge, challenge.tag);
    macDomain(secret, secretLength, FINISH_DOMAIN, sizeof(FINISH_DOMAIN) - 1U,
              transcript, expectedFinish);
    const bool finishValid = constantTimeTagEquals(expectedFinish, finish.tag);
    memset(expectedChallenge, 0, sizeof(expectedChallenge));
    memset(expectedFinish, 0, sizeof(expectedFinish));
    if (!challengeValid || !finishValid) {
        memset(transcript, 0, sizeof(transcript));
        return false;
    }
    macDomain(secret, secretLength, SESSION_DOMAIN, sizeof(SESSION_DOMAIN) - 1U,
              transcript, sessionKey);
    memset(transcript, 0, sizeof(transcript));
    return true;
}


PeerHandshakeResponder::PeerHandshakeResponder() : pending_ (false) {
    memset(&pendingHello_, 0, sizeof(pendingHello_));
    memset(&pendingChallenge_, 0, sizeof(pendingChallenge_));
}

bool PeerHandshakeResponder::begin(uint8_t expectedPeerDeviceId, uint8_t localDeviceId,
                                   uint32_t localBootId, const PeerHandshakeHello& hello,
                                   const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
                                   const uint8_t* secret, size_t secretLength,
                                   PeerHandshakeChallenge& challenge,
                                   uint32_t authenticatedReplayFloor) {
    if (pending_) return false;
    memset(&challenge, 0, sizeof(challenge));
    if (expectedPeerDeviceId == 0U || localDeviceId == 0U ||
        hello.initiatorDeviceId != expectedPeerDeviceId ||
        hello.responderDeviceId != localDeviceId || localBootId == 0U) return false;
    if (!createPeerHandshakeChallenge(hello, localBootId, responderNonce,
                                      secret, secretLength, challenge,
                                      authenticatedReplayFloor)) return false;
    pendingHello_ = hello;
    pendingChallenge_ = challenge;
    pending_ = true;
    return true;
}

bool PeerHandshakeResponder::beginWithReplayStore(
    uint8_t expectedPeerDeviceId, uint8_t localDeviceId, uint32_t localBootId,
    const PeerHandshakeHello& hello,
    const uint8_t responderNonce[PEER_HANDSHAKE_NONCE_SIZE],
    const uint8_t* secret, size_t secretLength, IPeerReplayStore& replayStore,
    PeerHandshakeChallenge& challenge) {
    uint32_t replayFloor = 0U;
    if (hello.initiatorDeviceId != expectedPeerDeviceId ||
        hello.responderDeviceId != localDeviceId ||
        !readPeerReplayFloorForHandshake(replayStore, localDeviceId,
                                         expectedPeerDeviceId,
                                         hello.initiatorBootId, replayFloor)) {
        memset(&challenge, 0, sizeof(challenge));
        return false;
    }
    return begin(expectedPeerDeviceId, localDeviceId, localBootId, hello,
                 responderNonce, secret, secretLength, challenge, replayFloor);
}

bool PeerHandshakeResponder::acceptFinish(const PeerHandshakeHello& hello,
                                          const PeerHandshakeChallenge& challenge,
                                          const PeerHandshakeFinish& finish,
                                          const uint8_t* secret, size_t secretLength,
                                          uint8_t sessionKey[PEER_HANDSHAKE_TAG_SIZE]) {
    if (sessionKey) memset(sessionKey, 0, PEER_HANDSHAKE_TAG_SIZE);
    if (!pending_) return false;
    const bool sameHello = hello.initiatorDeviceId == pendingHello_.initiatorDeviceId &&
        hello.responderDeviceId == pendingHello_.responderDeviceId &&
        hello.initiatorBootId == pendingHello_.initiatorBootId &&
        memcmp(hello.initiatorNonce, pendingHello_.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0;
    const bool sameChallenge = challenge.initiatorDeviceId == pendingChallenge_.initiatorDeviceId &&
        challenge.responderDeviceId == pendingChallenge_.responderDeviceId &&
        challenge.initiatorBootId == pendingChallenge_.initiatorBootId &&
        challenge.responderBootId == pendingChallenge_.responderBootId &&
        challenge.authenticatedReplayFloor == pendingChallenge_.authenticatedReplayFloor &&
        memcmp(challenge.initiatorNonce, pendingChallenge_.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0 &&
        memcmp(challenge.responderNonce, pendingChallenge_.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) == 0 &&
        constantTimeTagEquals(challenge.tag, pendingChallenge_.tag);
    const bool matches = sameHello && sameChallenge;
    // Consume before verification: a FINISH attempt can never be replayed on
    // this responder instance, including after a failed authentication.
    pending_ = false;
    bool accepted = false;
    if (matches)
        accepted = verifyPeerHandshakeFinish(pendingHello_, pendingChallenge_, finish,
                                             secret, secretLength, sessionKey);
    memset(&pendingHello_, 0, sizeof(pendingHello_));
    memset(&pendingChallenge_, 0, sizeof(pendingChallenge_));
    return accepted;
}

void PeerHandshakeResponder::cancel() {
    pending_ = false;
    memset(&pendingHello_, 0, sizeof(pendingHello_));
    memset(&pendingChallenge_, 0, sizeof(pendingChallenge_));
}

bool PeerHandshakeResponder::pending() const { return pending_; }

} // namespace HWC
