#include "PeerSessionCodec.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace HWC;

int main() {
    uint8_t session[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(session); ++i) session[i] = static_cast<uint8_t>(i + 1U);
    PeerDirectionalKeys keys;
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11223344UL, 0x55667788UL, keys));

    PeerSessionCodec initiator(keys, 1U, 2U, 0x11223344UL, 0x55667788UL, true);
    PeerSessionCodec responder(keys, 2U, 1U, 0x55667788UL, 0x11223344UL, false);
    assert(initiator.isValid() && responder.isValid());

    PeerCommandFrame command;
    memset(&command, 0, sizeof(command));
    command.frameType = PEER_FRAME_COMMAND;
    command.commandCode = 7U;
    command.senderDeviceId = 1U;
    command.targetDeviceId = 2U;
    command.senderBootId = 0x11223344UL;
    command.targetBootId = 0x55667788UL;
    command.sequence = 1U;
    command.requestId = 42U;
    command.argument0 = 123;

    uint8_t wire[PEER_COMMAND_FRAME_SIZE];
    size_t written = 0U;
    assert(initiator.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(written == PEER_COMMAND_FRAME_SIZE);
    PeerCommandFrame decoded;
    assert(responder.decodeIncoming(wire, written, decoded));
    assert(decoded.requestId == command.requestId && decoded.argument0 == 123);

    // The same wire frame is not accepted in the opposite direction or by a
    // context pinned to a different peer boot ID.
    PeerCommandFrame untouched;
    memset(&untouched, 0xA5, sizeof(untouched));
    assert(!initiator.decodeIncoming(wire, written, untouched));
    PeerSessionCodec stalePeer(keys, 2U, 1U, 0x55667788UL, 0x55667789UL, false);
    assert(!stalePeer.decodeIncoming(wire, written, untouched));

    // A responder-originated response uses the opposite directional key.
    PeerCommandFrame response;
    memset(&response, 0, sizeof(response));
    response.frameType = PEER_FRAME_RESPONSE;
    response.commandCode = 0x81U;
    response.senderDeviceId = 2U;
    response.targetDeviceId = 1U;
    response.senderBootId = 0x55667788UL;
    response.targetBootId = 0x11223344UL;
    response.sequence = 1U;
    response.requestId = 42U;
    assert(responder.encodeOutgoing(response, wire, sizeof(wire), written));
    assert(initiator.decodeIncoming(wire, written, decoded));
    assert(decoded.frameType == PEER_FRAME_RESPONSE && decoded.requestId == 42U);

    // An initiator cannot forge a responder frame through its outgoing API.
    assert(!initiator.encodeOutgoing(response, wire, sizeof(wire), written));

    PeerSessionCodec invalid(keys, 1U, 1U, 1U, 2U, true);
    assert(!invalid.isValid());
    assert(!invalid.encodeOutgoing(command, wire, sizeof(wire), written));

    clearPeerDirectionalKeys(keys);
    puts("PASS: PeerSessionCodec directional authentication and peer binding tests");
    return 0;
}
