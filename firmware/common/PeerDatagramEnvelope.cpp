#include "PeerDatagramEnvelope.h"
#include <string.h>

namespace HWC {
namespace {
bool validType(uint8_t type) {
    return type >= static_cast<uint8_t>(PEER_DATAGRAM_HELLO) &&
           type <= static_cast<uint8_t>(PEER_DATAGRAM_STATUS);
}
}

bool encodePeerDatagram(PeerDatagramType type, const uint8_t* payload,
                        size_t payloadLength, uint8_t* output,
                        size_t capacity, size_t& written) {
    written = 0U;
    const uint8_t rawType = static_cast<uint8_t>(type);
    if (!validType(rawType) || payload == 0 || output == 0 ||
        payloadLength == 0U || payloadLength > PEER_DATAGRAM_MAX_PAYLOAD ||
        payloadLength > 0xFFFFU || capacity < PEER_DATAGRAM_HEADER_SIZE + payloadLength) {
        return false;
    }
    output[0] = PEER_DATAGRAM_MAGIC_0;
    output[1] = PEER_DATAGRAM_MAGIC_1;
    output[2] = PEER_DATAGRAM_VERSION;
    output[3] = rawType;
    output[4] = static_cast<uint8_t>((payloadLength >> 8) & 0xFFU);
    output[5] = static_cast<uint8_t>(payloadLength & 0xFFU);
    output[6] = 0U;
    output[7] = 0U;
    memcpy(output + PEER_DATAGRAM_HEADER_SIZE, payload, payloadLength);
    written = PEER_DATAGRAM_HEADER_SIZE + payloadLength;
    return true;
}

bool decodePeerDatagram(const uint8_t* data, size_t length,
                        PeerDatagramView& view) {
    view.type = PEER_DATAGRAM_HELLO;
    view.payload = 0;
    view.payloadLength = 0U;
    if (data == 0 || length < PEER_DATAGRAM_HEADER_SIZE ||
        data[0] != PEER_DATAGRAM_MAGIC_0 || data[1] != PEER_DATAGRAM_MAGIC_1 ||
        data[2] != PEER_DATAGRAM_VERSION || !validType(data[3]) ||
        data[6] != 0U || data[7] != 0U) return false;

    const size_t payloadLength = (static_cast<size_t>(data[4]) << 8) | data[5];
    if (payloadLength == 0U || payloadLength > PEER_DATAGRAM_MAX_PAYLOAD ||
        length != PEER_DATAGRAM_HEADER_SIZE + payloadLength) return false;

    view.type = static_cast<PeerDatagramType>(data[3]);
    view.payload = data + PEER_DATAGRAM_HEADER_SIZE;
    view.payloadLength = payloadLength;
    return true;
}

} // namespace HWC
