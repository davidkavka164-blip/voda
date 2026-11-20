#include "PeerCommandResponse.h"
#include "HmacSha256.h"
#include <assert.h>
#include <string.h>
using namespace HWC;

int main() {
    PeerCommandFrame request;
    memset(&request, 0, sizeof(request));
    request.frameType = PEER_FRAME_COMMAND;
    request.commandCode = 3;
    request.senderDeviceId = 1;
    request.targetDeviceId = 2;
    request.senderBootId = 101;
    request.targetBootId = 202;
    request.sequence = 7;
    request.requestId = 9001;
    request.argument0 = 1200;

    PeerCommandFrame response;
    assert(buildPeerCommandResponse(request, 2, 202, 8, PEER_RESPONSE_ACK,
                                    PEER_REASON_NONE, response));
    assert(response.frameType == PEER_FRAME_RESPONSE);
    assert(response.requestId == request.requestId);
    assert(peerCommandResponseMatches(request, response));
    PeerCommandResponse parsed;
    assert(parsePeerCommandResponse(response, parsed));
    assert(parsed.code == PEER_RESPONSE_ACK && parsed.originalCommandCode == 3);
    assert(parsed.reason == PEER_REASON_NONE);
    const uint8_t key[32] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,
                             17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32};
    uint8_t wire[PEER_COMMAND_FRAME_SIZE];
    size_t wireLength = 0;
    assert(encodePeerCommandFrame(response, key, sizeof(key), wire, sizeof(wire), wireLength));
    PeerCommandFrame decoded;
    assert(decodePeerCommandFrame(wire, wireLength, key, sizeof(key), decoded));
    assert(peerCommandResponseMatches(request, decoded));
    wire[30] ^= 0x01U;
    assert(!decodePeerCommandFrame(wire, wireLength, key, sizeof(key), decoded));

    assert(buildPeerCommandResponse(request, 2, 202, 9, PEER_RESPONSE_REJECTED,
                                    PEER_REASON_TARGET_FULL, response));
    assert(peerCommandResponseMatches(request, response));
    assert(parsePeerCommandResponse(response, parsed));
    assert(parsed.code == PEER_RESPONSE_REJECTED && parsed.reason == PEER_REASON_TARGET_FULL);

    assert(buildPeerCommandResponse(request, 2, 202, 10, PEER_RESPONSE_FAILED,
                                    PEER_REASON_COMMUNICATION_LOST, response));
    assert(peerCommandResponseMatches(request, response));

    assert(!buildPeerCommandResponse(request, 2, 202, 11, PEER_RESPONSE_ACK,
                                     PEER_REASON_TIMEOUT, response));
    assert(!buildPeerCommandResponse(request, 2, 202, 11, PEER_RESPONSE_FAILED,
                                     PEER_REASON_NONE, response));
    assert(!buildPeerCommandResponse(request, 1, 101, 11, PEER_RESPONSE_ACK,
                                     PEER_REASON_NONE, response));
    assert(!buildPeerCommandResponse(request, 2, 999, 11, PEER_RESPONSE_ACK,
                                     PEER_REASON_NONE, response));
    assert(!buildPeerCommandResponse(request, 2, 202, 0, PEER_RESPONSE_ACK,
                                     PEER_REASON_NONE, response));

    assert(buildPeerCommandResponse(request, 2, 202, 12, PEER_RESPONSE_ACK,
                                    PEER_REASON_NONE, response));
    response.requestId++;
    assert(!peerCommandResponseMatches(request, response));
    response.requestId = request.requestId;
    response.targetBootId++;
    assert(!peerCommandResponseMatches(request, response));
    response.targetBootId = request.senderBootId;
    response.argument0 = 99;
    assert(!peerCommandResponseMatches(request, response));
    response.argument0 = request.commandCode;
    response.commandCode = 0x90;
    assert(!parsePeerCommandResponse(response, parsed));
    return 0;
}
