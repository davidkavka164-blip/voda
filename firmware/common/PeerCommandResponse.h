#ifndef HWC_PEER_COMMAND_RESPONSE_H
#define HWC_PEER_COMMAND_RESPONSE_H

#include "PeerCommandFrame.h"
#include <stdint.h>

namespace HWC {

enum PeerResponseCode : uint8_t {
    PEER_RESPONSE_ACK = 0x81U,
    PEER_RESPONSE_REJECTED = 0x82U,
    PEER_RESPONSE_FAILED = 0x83U
};

enum PeerResponseReason : int32_t {
    PEER_REASON_NONE = 0,
    PEER_REASON_INVALID_REQUEST = 1,
    PEER_REASON_UNSAFE_LEVEL = 2,
    PEER_REASON_SOURCE_EMPTY = 3,
    PEER_REASON_TARGET_FULL = 4,
    PEER_REASON_PROTECTION_LATCHED = 5,
    PEER_REASON_SENSOR_INVALID = 6,
    PEER_REASON_CONFIGURATION_INVALID = 7,
    PEER_REASON_STORAGE_FAULT = 8,
    PEER_REASON_BUSY = 9,
    PEER_REASON_COMMUNICATION_LOST = 10,
    PEER_REASON_TIMEOUT = 11,
    PEER_REASON_OUTPUT_NOT_READY = 12,
    PEER_REASON_INTERNAL_ERROR = 13
};

struct PeerCommandResponse {
    PeerResponseCode code;
    uint8_t originalCommandCode;
    PeerResponseReason reason;
};

// Response uses the authenticated PeerCommandFrame envelope:
// commandCode=response code, argument0=original command code,
// argument1=reason. requestId is copied from the request.
bool buildPeerCommandResponse(const PeerCommandFrame& request,
                              uint8_t responderDeviceId,
                              uint32_t responderBootId,
                              uint32_t responseSequence,
                              PeerResponseCode code,
                              PeerResponseReason reason,
                              PeerCommandFrame& response);

bool parsePeerCommandResponse(const PeerCommandFrame& frame,
                              PeerCommandResponse& response);

// Checks that an authenticated response is for this exact outstanding request.
bool peerCommandResponseMatches(const PeerCommandFrame& request,
                                const PeerCommandFrame& response);

} // namespace HWC
#endif
