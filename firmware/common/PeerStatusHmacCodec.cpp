#include "PeerStatusHmacCodec.h"
#include "HmacSha256.h"
#include <string.h>

namespace HWC {
namespace {
bool validKey(const uint8_t* key, size_t length) {
    if (key == 0 || length != PEER_STATUS_HMAC_KEY_SIZE) return false;
    uint8_t aggregate = 0U;
    for (size_t i = 0; i < length; ++i) aggregate |= key[i];
    return aggregate != 0U;
}
}

bool encodePeerStatusHmacFrame(const PeerStatusFrame& frame,
                               uint8_t expectedDeviceId,
                               const uint8_t* sharedKey, size_t keyLength,
                               uint8_t* output, size_t capacity,
                               size_t& written) {
    written = 0U;
    if (!validKey(sharedKey, keyLength) || output == 0 ||
        capacity < PEER_STATUS_HMAC_WIRE_SIZE ||
        !isPeerStatusFrameValid(frame, expectedDeviceId)) return false;

    size_t payloadLength = 0U;
    if (!encodePeerStatusFrame(frame, output, PEER_STATUS_WIRE_SIZE,
                               payloadLength) ||
        payloadLength != PEER_STATUS_WIRE_SIZE) return false;

    hmacSha256(sharedKey, keyLength, output, PEER_STATUS_WIRE_SIZE,
               output + PEER_STATUS_WIRE_SIZE);
    written = PEER_STATUS_HMAC_WIRE_SIZE;
    return true;
}

bool decodePeerStatusHmacFrame(const uint8_t* data, size_t length,
                               uint8_t expectedDeviceId,
                               const uint8_t* sharedKey, size_t keyLength,
                               PeerStatusFrame& frame) {
    memset(&frame, 0, sizeof(frame));
    if (!validKey(sharedKey, keyLength) || data == 0 ||
        length != PEER_STATUS_HMAC_WIRE_SIZE) return false;

    uint8_t expectedTag[HMAC_SHA256_SIZE] = {0U};
    hmacSha256(sharedKey, keyLength, data, PEER_STATUS_WIRE_SIZE,
               expectedTag);
    const bool authenticated = constantTimeTagEquals(
        expectedTag, data + PEER_STATUS_WIRE_SIZE);
    volatile uint8_t* clear = expectedTag;
    for (size_t i = 0; i < sizeof(expectedTag); ++i) clear[i] = 0U;
    if (!authenticated) return false;

    PeerStatusFrame decoded = {};
    if (!decodePeerStatusFrame(data, PEER_STATUS_WIRE_SIZE,
                               expectedDeviceId, decoded)) return false;
    frame = decoded;
    return true;
}

} // namespace HWC
