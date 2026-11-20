#ifndef HWC_PEER_DATAGRAM_ENVELOPE_H
#define HWC_PEER_DATAGRAM_ENVELOPE_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

// Small, canonical transport discriminator. The payload remains encoded by
// its existing HELLO/CHALLENGE/FINISH/READY/status codec.
static const uint8_t PEER_DATAGRAM_MAGIC_0 = 0x48U; // 'H'
static const uint8_t PEER_DATAGRAM_MAGIC_1 = 0x57U; // 'W'
static const uint8_t PEER_DATAGRAM_VERSION = 1U;
static const size_t PEER_DATAGRAM_HEADER_SIZE = 8U;
static const size_t PEER_DATAGRAM_MAX_PAYLOAD = 256U;

enum PeerDatagramType : uint8_t {
    PEER_DATAGRAM_HELLO = 1U,
    PEER_DATAGRAM_CHALLENGE = 2U,
    PEER_DATAGRAM_FINISH = 3U,
    PEER_DATAGRAM_READY = 4U,
    PEER_DATAGRAM_STATUS = 5U
};

struct PeerDatagramView {
    PeerDatagramType type;
    const uint8_t* payload;
    size_t payloadLength;
};

// Wire header: magic[2], version[1], type[1], payload length BE[2],
// reserved[2] (must be zero). No native structs are transmitted.
bool encodePeerDatagram(PeerDatagramType type, const uint8_t* payload,
                        size_t payloadLength, uint8_t* output,
                        size_t capacity, size_t& written);
bool decodePeerDatagram(const uint8_t* data, size_t length,
                        PeerDatagramView& view);

} // namespace HWC
#endif
