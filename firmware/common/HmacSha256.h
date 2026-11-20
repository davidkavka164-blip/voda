#ifndef HWC_HMAC_SHA256_H
#define HWC_HMAC_SHA256_H
#include <stddef.h>
#include <stdint.h>
namespace HWC {
static const size_t HMAC_SHA256_SIZE = 32U;
// Computes standard HMAC-SHA-256. Caller owns key provisioning and message canonicalization.
void hmacSha256(const uint8_t* key, size_t keyLength,
                const uint8_t* message, size_t messageLength,
                uint8_t output[HMAC_SHA256_SIZE]);
// Constant-time for equal-length 32-byte tags.
bool constantTimeTagEquals(const uint8_t a[HMAC_SHA256_SIZE],
                           const uint8_t b[HMAC_SHA256_SIZE]);
}
#endif
