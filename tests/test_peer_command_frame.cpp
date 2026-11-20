#include "PeerCommandFrame.h"
#include "PeerSessionKeys.h"
#include <assert.h>
#include <string.h>

int main() {
    uint8_t key[32];
    for (unsigned i = 0; i < 32; ++i) key[i] = static_cast<uint8_t>(i + 1U);

    HWC::PeerCommandFrame frame = {};
    frame.frameType = HWC::PEER_FRAME_COMMAND;
    frame.commandCode = 3U;
    frame.senderDeviceId = 1U;
    frame.targetDeviceId = 2U;
    frame.senderBootId = 0x11223344U;
    frame.targetBootId = 0x55667788U;
    frame.sequence = 9U;
    frame.requestId = 1234U;
    frame.argument0 = 2500;
    frame.argument1 = -17;

    uint8_t wire[HWC::PEER_COMMAND_FRAME_SIZE];
    size_t written = 0U;
    assert(HWC::encodePeerCommandFrame(frame, key, sizeof(key), wire, sizeof(wire), written));
    assert(written == sizeof(wire));
    HWC::PeerCommandFrame decoded = {};
    assert(HWC::decodePeerCommandFrame(wire, written, key, sizeof(key), decoded));
    assert(decoded.frameType == frame.frameType && decoded.commandCode == frame.commandCode &&
           decoded.senderBootId == frame.senderBootId && decoded.argument0 == frame.argument0 &&
           decoded.argument1 == frame.argument1);

    uint8_t bad[HWC::PEER_COMMAND_FRAME_SIZE];
    memcpy(bad, wire, sizeof(bad));
    bad[28] ^= 1U;
    assert(!HWC::decodePeerCommandFrame(bad, sizeof(bad), key, sizeof(key), decoded));
    assert(!HWC::decodePeerCommandFrame(wire, sizeof(wire) - 1U, key, sizeof(key), decoded));
    assert(!HWC::decodePeerCommandFrame(wire, sizeof(wire), key, 31U, decoded));
    assert(!HWC::encodePeerCommandFrame(frame, key, 31U, wire, sizeof(wire), written));

    // A command is authenticated with the initiator->responder key only.
    HWC::PeerDirectionalKeys directional = {};
    assert(HWC::derivePeerDirectionalKeys(key, sizeof(key), 1U, 2U,
                                          0x11223344U, 0x55667788U, directional));
    frame.requestId = 1235U;
    assert(HWC::encodePeerCommandFrame(frame, directional.initiatorToResponder,
                                       sizeof(directional.initiatorToResponder),
                                       wire, sizeof(wire), written));
    assert(HWC::decodePeerCommandFrame(wire, written, directional.initiatorToResponder,
                                       sizeof(directional.initiatorToResponder), decoded));
    assert(!HWC::decodePeerCommandFrame(wire, written, directional.responderToInitiator,
                                        sizeof(directional.responderToInitiator), decoded));
    HWC::clearPeerDirectionalKeys(directional);

    frame.requestId = 0U;
    assert(!HWC::encodePeerCommandFrame(frame, key, sizeof(key), wire, sizeof(wire), written));
    return 0;
}
