#include "PeerCommandSession.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

using namespace HWC;

static PeerCommandFrame makeCommand(uint32_t sequence, uint32_t requestId) {
    PeerCommandFrame f;
    memset(&f, 0, sizeof(f));
    f.frameType = PEER_FRAME_COMMAND;
    f.commandCode = 7U;
    f.senderDeviceId = 1U;
    f.targetDeviceId = 2U;
    f.senderBootId = 0x11223344UL;
    f.targetBootId = 0x55667788UL;
    f.sequence = sequence;
    f.requestId = requestId;
    f.argument0 = 125;
    return f;
}

int main() {
    uint8_t sessionKey[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(sessionKey); ++i)
        sessionKey[i] = static_cast<uint8_t>(i + 3U);
    PeerDirectionalKeys keys;
    assert(derivePeerDirectionalKeys(sessionKey, sizeof(sessionKey), 1U, 2U,
                                     0x11223344UL, 0x55667788UL, keys));

    PeerCommandSession initiator;
    PeerCommandSession responder;
    assert(!responder.activate(keys, 2U, 1U, 0x55667788UL, 0x11223344UL,
                               false, false));
    assert(!responder.isActive());
    assert(initiator.activate(keys, 1U, 2U, 0x11223344UL, 0x55667788UL,
                              true, true));
    assert(responder.activate(keys, 2U, 1U, 0x55667788UL, 0x11223344UL,
                              false, true));

    uint8_t wire[PEER_COMMAND_FRAME_SIZE];
    size_t written = 0U;
    PeerCommandFrame accepted;
    PeerCommandFrame command = makeCommand(1U, 77U);
    assert(initiator.prepareOutgoingRequest(command, 100U, 1000U,
                                            wire, sizeof(wire), written) ==
           PEER_REQUEST_TRACKED);
    assert(initiator.pendingRequestCount() == 1U);
    assert(responder.receiveCommand(wire, written, accepted) == PEER_COMMAND_ACCEPTED);
    assert(accepted.requestId == 77U && accepted.argument0 == 125);
    assert(responder.acceptedCommandCount() == 1U);

    // Response must pass HMAC/session validation before request correlation.
    PeerCommandFrame responseFrame;
    assert(buildPeerCommandResponse(accepted, 2U, 0x55667788UL, 1U,
                                    PEER_RESPONSE_ACK, PEER_REASON_NONE,
                                    responseFrame));
    uint8_t responseWire[PEER_COMMAND_FRAME_SIZE];
    size_t responseWritten = 0U;
    assert(responder.encodeOutgoing(responseFrame, responseWire,
                                    sizeof(responseWire), responseWritten));
    PeerCommandResponse parsedResponse;
    uint8_t tampered[PEER_COMMAND_FRAME_SIZE];
    memcpy(tampered, responseWire, responseWritten);
    tampered[12] ^= 0x01U;
    assert(initiator.receiveResponse(tampered, responseWritten, 150U,
                                     parsedResponse) == PEER_REQUEST_INVALID);
    assert(initiator.pendingRequestCount() == 1U);
    assert(initiator.receiveResponse(responseWire, responseWritten, 151U,
                                     parsedResponse) == PEER_REQUEST_RESPONSE_MATCHED);
    assert(parsedResponse.code == PEER_RESPONSE_ACK);
    assert(initiator.pendingRequestCount() == 0U);

    // A completed request ID cannot be reused while delayed responses may exist.
    command = makeCommand(2U, 77U);
    memset(wire, 0xA5, sizeof(wire));
    assert(initiator.prepareOutgoingRequest(command, 200U, 1000U,
                                            wire, sizeof(wire), written) ==
           PEER_REQUEST_INVALID);
    assert(written == 0U);

    // A new request is tracked and can be timed out.
    command = makeCommand(2U, 78U);
    memset(wire, 0xA5, sizeof(wire));
    assert(initiator.prepareOutgoingRequest(command, 200U, 1000U,
                                            wire, sizeof(wire), written) ==
           PEER_REQUEST_TRACKED);
    assert(written == PEER_COMMAND_FRAME_SIZE);
    uint32_t expiredRequestId = 0U;
    assert(initiator.takeExpiredRequest(1200U, expiredRequestId));
    assert(expiredRequestId == 78U);
    assert(initiator.pendingRequestCount() == 0U);
    assert(responder.receiveCommand(wire, written, accepted) == PEER_COMMAND_ACCEPTED);

    // Exact replay is rejected and does not leak decoded data to the caller.
    memset(&accepted, 0xA5, sizeof(accepted));
    assert(responder.receiveCommand(wire, written, accepted) ==
           PEER_COMMAND_REJECT_DUPLICATE_REQUEST);
    PeerCommandFrame zero;
    memset(&zero, 0, sizeof(zero));
    assert(memcmp(&accepted, &zero, sizeof(zero)) == 0);

    // A new sequence with a reused request ID is also rejected.
    command = makeCommand(2U, 77U);
    assert(initiator.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(responder.receiveCommand(wire, written, accepted) ==
           PEER_COMMAND_REJECT_DUPLICATE_REQUEST);

    // A response cannot enter the command-admission path.
    command.frameType = PEER_FRAME_RESPONSE;
    command.commandCode = 0x81U;
    command.sequence = 3U;
    assert(initiator.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(responder.receiveCommand(wire, written, accepted) ==
           PEER_COMMAND_REJECT_WRONG_FRAME_TYPE);

    // Replayed old wire data remains invalid after a new session because the
    // codec is cleared and a different session key is required.
    responder.close();
    assert(!responder.isActive());
    assert(responder.receiveCommand(wire, written, accepted) ==
           PEER_COMMAND_REJECT_UNESTABLISHED_SESSION);
    assert(!responder.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(written == 0U);

    // A fresh authenticated session on the same running device resets sequence
    // ordering but retains recent request IDs across the link/session boundary.
    assert(responder.activate(keys, 2U, 1U, 0x55667788UL, 0x11223344UL,
                              false, true));
    command = makeCommand(1U, 77U);
    assert(initiator.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(responder.receiveCommand(wire, written, accepted) ==
           PEER_COMMAND_REJECT_DUPLICATE_REQUEST);
    command = makeCommand(1U, 79U);
    assert(initiator.encodeOutgoing(command, wire, sizeof(wire), written));
    assert(responder.receiveCommand(wire, written, accepted) == PEER_COMMAND_ACCEPTED);

    clearPeerDirectionalKeys(keys);
    memset(sessionKey, 0, sizeof(sessionKey));
    puts("PASS: PeerCommandSession authenticated admission, replay and close tests");
    return 0;
}
