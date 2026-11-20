#include "PeerHandshakeReady.h"
#include "HmacSha256.h"
#include <string.h>

namespace HWC {
namespace {
static const uint8_t READY_DOMAIN[] = "HWC-HS-READY-V1";
}

bool createPeerHandshakeReady(const uint8_t* sessionKey, size_t keyLength,
                              PeerHandshakeReady& ready) {
    memset(&ready, 0, sizeof(ready));
    if (sessionKey == 0 || keyLength != PEER_HANDSHAKE_TAG_SIZE) return false;
    hmacSha256(sessionKey, keyLength, READY_DOMAIN, sizeof(READY_DOMAIN) - 1U, ready.tag);
    return true;
}

bool verifyPeerHandshakeReady(const uint8_t* sessionKey, size_t keyLength,
                              const PeerHandshakeReady& ready) {
    if (sessionKey == 0 || keyLength != PEER_HANDSHAKE_TAG_SIZE) return false;
    uint8_t expected[HMAC_SHA256_SIZE];
    hmacSha256(sessionKey, keyLength, READY_DOMAIN, sizeof(READY_DOMAIN) - 1U, expected);
    const bool valid = constantTimeTagEquals(expected, ready.tag);
    memset(expected, 0, sizeof(expected));
    return valid;
}

} // namespace HWC
