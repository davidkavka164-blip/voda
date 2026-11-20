#include "PeerCommandResponse.h"
#include <string.h>

namespace HWC {
namespace {
bool validCode(PeerResponseCode code) {
    return code == PEER_RESPONSE_ACK || code == PEER_RESPONSE_REJECTED ||
           code == PEER_RESPONSE_FAILED;
}
bool validReason(PeerResponseReason reason) {
    return reason >= PEER_REASON_NONE && reason <= PEER_REASON_INTERNAL_ERROR;
}
bool validCombination(PeerResponseCode code, PeerResponseReason reason) {
    if (!validCode(code) || !validReason(reason)) return false;
    if (code == PEER_RESPONSE_ACK) return reason == PEER_REASON_NONE;
    return reason != PEER_REASON_NONE;
}
}

bool buildPeerCommandResponse(const PeerCommandFrame& request,
                              uint8_t responderDeviceId,
                              uint32_t responderBootId,
                              uint32_t responseSequence,
                              PeerResponseCode code,
                              PeerResponseReason reason,
                              PeerCommandFrame& response) {
    memset(&response, 0, sizeof(response));
    if (request.frameType != PEER_FRAME_COMMAND || request.commandCode == 0U ||
        request.senderDeviceId == 0U || request.targetDeviceId == 0U ||
        request.senderBootId == 0U || request.targetBootId == 0U ||
        request.sequence == 0U || request.requestId == 0U ||
        responderDeviceId != request.targetDeviceId ||
        responderBootId != request.targetBootId || responseSequence == 0U ||
        !validCombination(code, reason)) return false;

    response.frameType = PEER_FRAME_RESPONSE;
    response.commandCode = static_cast<uint8_t>(code);
    response.senderDeviceId = responderDeviceId;
    response.targetDeviceId = request.senderDeviceId;
    response.senderBootId = responderBootId;
    response.targetBootId = request.senderBootId;
    response.sequence = responseSequence;
    response.requestId = request.requestId;
    response.argument0 = static_cast<int32_t>(request.commandCode);
    response.argument1 = static_cast<int32_t>(reason);
    return true;
}

bool parsePeerCommandResponse(const PeerCommandFrame& frame,
                              PeerCommandResponse& response) {
    if (frame.frameType != PEER_FRAME_RESPONSE ||
        frame.argument0 <= 0 || frame.argument0 > 255 ||
        frame.argument1 < PEER_REASON_NONE || frame.argument1 > PEER_REASON_INTERNAL_ERROR)
        return false;
    const PeerResponseCode code = static_cast<PeerResponseCode>(frame.commandCode);
    const PeerResponseReason reason = static_cast<PeerResponseReason>(frame.argument1);
    if (!validCombination(code, reason)) return false;
    response.code = code;
    response.originalCommandCode = static_cast<uint8_t>(frame.argument0);
    response.reason = reason;
    return true;
}

bool peerCommandResponseMatches(const PeerCommandFrame& request,
                                const PeerCommandFrame& response) {
    PeerCommandResponse parsed;
    if (request.frameType != PEER_FRAME_COMMAND ||
        !parsePeerCommandResponse(response, parsed)) return false;
    return response.requestId == request.requestId &&
           response.senderDeviceId == request.targetDeviceId &&
           response.targetDeviceId == request.senderDeviceId &&
           response.senderBootId == request.targetBootId &&
           response.targetBootId == request.senderBootId &&
           parsed.originalCommandCode == request.commandCode;
}

} // namespace HWC
