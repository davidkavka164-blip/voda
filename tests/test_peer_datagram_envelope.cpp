#include "PeerDatagramEnvelope.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    uint8_t payload[84];
    for (size_t i = 0; i < sizeof(payload); ++i) payload[i] = static_cast<uint8_t>(i);
    uint8_t wire[PEER_DATAGRAM_HEADER_SIZE + sizeof(payload)];
    size_t written = 0U;
    assert(encodePeerDatagram(PEER_DATAGRAM_CHALLENGE, payload, sizeof(payload),
                              wire, sizeof(wire), written));
    assert(written == sizeof(wire));

    PeerDatagramView view;
    assert(decodePeerDatagram(wire, written, view));
    assert(view.type == PEER_DATAGRAM_CHALLENGE);
    assert(view.payloadLength == sizeof(payload));
    assert(memcmp(view.payload, payload, sizeof(payload)) == 0);

    // Same 84-byte payload length as authenticated status is unambiguous
    // because the envelope carries an explicit datagram type.
    assert(encodePeerDatagram(PEER_DATAGRAM_STATUS, payload, sizeof(payload),
                              wire, sizeof(wire), written));
    assert(decodePeerDatagram(wire, written, view));
    assert(view.type == PEER_DATAGRAM_STATUS);

    assert(!decodePeerDatagram(wire, written - 1U, view));
    assert(!decodePeerDatagram(wire, written + 1U, view));
    uint8_t saved = wire[2]; wire[2] = 2U;
    assert(!decodePeerDatagram(wire, written, view)); wire[2] = saved;
    saved = wire[3]; wire[3] = 99U;
    assert(!decodePeerDatagram(wire, written, view)); wire[3] = saved;
    saved = wire[6]; wire[6] = 1U;
    assert(!decodePeerDatagram(wire, written, view)); wire[6] = saved;
    saved = wire[0]; wire[0] ^= 0x01U;
    assert(!decodePeerDatagram(wire, written, view)); wire[0] = saved;

    written = 123U;
    assert(!encodePeerDatagram(PEER_DATAGRAM_STATUS, payload, sizeof(payload),
                               wire, PEER_DATAGRAM_HEADER_SIZE, written));
    assert(written == 0U);
    assert(!encodePeerDatagram(static_cast<PeerDatagramType>(0U), payload,
                               sizeof(payload), wire, sizeof(wire), written));
    assert(!encodePeerDatagram(PEER_DATAGRAM_STATUS, 0, sizeof(payload),
                               wire, sizeof(wire), written));
    puts("PASS: peer datagram envelope type/version/length validation and 84B collision prevention");
    return 0;
}
