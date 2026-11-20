#include "PeerStatusHmacCodec.h"
#include <assert.h>
#include <string.h>

using namespace HWC;

int main() {
    uint8_t key[PEER_STATUS_HMAC_KEY_SIZE];
    for (size_t i = 0; i < sizeof(key); ++i) key[i] = static_cast<uint8_t>(i + 1U);

    PeerStatusFrame source = makeEmptyPeerStatus(0x12345678UL, 42U, 9000U);
    source.upperLevelCm = 123.5f;
    source.upperLevelValid = 1U;
    updatePeerStatusChecksum(source);

    uint8_t wire[PEER_STATUS_HMAC_WIRE_SIZE] = {0U};
    size_t written = 0U;
    assert(encodePeerStatusHmacFrame(source, PEER_DEVICE_UPPER_TANK,
                                     key, sizeof(key), wire, sizeof(wire), written));
    assert(written == PEER_STATUS_HMAC_WIRE_SIZE);

    PeerStatusFrame decoded = {};
    assert(decodePeerStatusHmacFrame(wire, written, PEER_DEVICE_UPPER_TANK,
                                     key, sizeof(key), decoded));
    assert(decoded.bootId == source.bootId);
    assert(decoded.sequence == source.sequence);
    assert(decoded.upperLevelCm == source.upperLevelCm);

    uint8_t wrongKey[PEER_STATUS_HMAC_KEY_SIZE];
    memcpy(wrongKey, key, sizeof(key));
    wrongKey[0] ^= 0x80U;
    assert(!decodePeerStatusHmacFrame(wire, written, PEER_DEVICE_UPPER_TANK,
                                      wrongKey, sizeof(wrongKey), decoded));

    uint8_t changed[PEER_STATUS_HMAC_WIRE_SIZE];
    memcpy(changed, wire, sizeof(changed));
    changed[20] ^= 0x01U;
    assert(!decodePeerStatusHmacFrame(changed, sizeof(changed), PEER_DEVICE_UPPER_TANK,
                                      key, sizeof(key), decoded));

    assert(!decodePeerStatusHmacFrame(wire, written - 1U, PEER_DEVICE_UPPER_TANK,
                                      key, sizeof(key), decoded));
    assert(!decodePeerStatusHmacFrame(wire, written + 1U, PEER_DEVICE_UPPER_TANK,
                                      key, sizeof(key), decoded));
    assert(!decodePeerStatusHmacFrame(wire, written, 1U, key, sizeof(key), decoded));
    assert(!decodePeerStatusHmacFrame(wire, written, PEER_DEVICE_UPPER_TANK,
                                      key, sizeof(key) - 1U, decoded));

    uint8_t zeroKey[PEER_STATUS_HMAC_KEY_SIZE] = {0U};
    assert(!encodePeerStatusHmacFrame(source, PEER_DEVICE_UPPER_TANK,
                                      zeroKey, sizeof(zeroKey), wire, sizeof(wire), written));
    assert(written == 0U);

    source.sequence++;
    assert(!encodePeerStatusHmacFrame(source, PEER_DEVICE_UPPER_TANK,
                                      key, sizeof(key), wire, PEER_STATUS_HMAC_WIRE_SIZE - 1U,
                                      written));
    assert(written == 0U);
    return 0;
}
