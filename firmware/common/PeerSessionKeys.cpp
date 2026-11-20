#include "PeerSessionKeys.h"
#include "HmacSha256.h"
#include <string.h>

namespace HWC { namespace {
static const uint8_t I2R_DOMAIN[] = "HWC-PEER-I2R-KEY-V1";
static const uint8_t R2I_DOMAIN[] = "HWC-PEER-R2I-KEY-V1";
static const size_t CONTEXT_SIZE = 10U;

void put32(uint8_t* out, uint32_t value) {
    out[0] = static_cast<uint8_t>(value);
    out[1] = static_cast<uint8_t>(value >> 8U);
    out[2] = static_cast<uint8_t>(value >> 16U);
    out[3] = static_cast<uint8_t>(value >> 24U);
}

void derive(const uint8_t* sessionKey, size_t sessionKeyLength,
            const uint8_t* domain, size_t domainLength,
            const uint8_t context[CONTEXT_SIZE], uint8_t output[HMAC_SHA256_SIZE]) {
    uint8_t message[sizeof(I2R_DOMAIN) - 1U + CONTEXT_SIZE];
    memcpy(message, domain, domainLength);
    memcpy(message + domainLength, context, CONTEXT_SIZE);
    hmacSha256(sessionKey, sessionKeyLength, message, domainLength + CONTEXT_SIZE, output);
    volatile uint8_t* p = message;
    for (size_t i = 0; i < sizeof(message); ++i) p[i] = 0U;
}
} // namespace

void clearPeerDirectionalKeys(PeerDirectionalKeys& keys) {
    volatile uint8_t* p = reinterpret_cast<volatile uint8_t*>(&keys);
    for (size_t i = 0; i < sizeof(keys); ++i) p[i] = 0U;
}

bool derivePeerDirectionalKeys(const uint8_t* sessionKey, size_t sessionKeyLength,
                               uint8_t initiatorDeviceId, uint8_t responderDeviceId,
                               uint32_t initiatorBootId, uint32_t responderBootId,
                               PeerDirectionalKeys& keys) {
    clearPeerDirectionalKeys(keys);
    if (sessionKey == 0 || sessionKeyLength != PEER_SESSION_KEY_SIZE ||
        initiatorDeviceId == 0U || responderDeviceId == 0U ||
        initiatorDeviceId == responderDeviceId ||
        initiatorBootId == 0U || responderBootId == 0U) return false;

    uint8_t context[CONTEXT_SIZE];
    context[0] = initiatorDeviceId;
    context[1] = responderDeviceId;
    put32(context + 2U, initiatorBootId);
    put32(context + 6U, responderBootId);

    derive(sessionKey, sessionKeyLength, I2R_DOMAIN, sizeof(I2R_DOMAIN) - 1U,
           context, keys.initiatorToResponder);
    derive(sessionKey, sessionKeyLength, R2I_DOMAIN, sizeof(R2I_DOMAIN) - 1U,
           context, keys.responderToInitiator);
    volatile uint8_t* p = context;
    for (size_t i = 0; i < sizeof(context); ++i) p[i] = 0U;
    return true;
}

} // namespace HWC
