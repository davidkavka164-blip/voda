#include "PeerStatusSessionCodec.h"
#include "HmacSha256.h"
#include <string.h>

namespace HWC {

PeerStatusSessionCodec::PeerStatusSessionCodec()
    : localDeviceId_(0U), peerDeviceId_(0U), localBootId_(0U),
      peerBootId_(0U), localIsInitiator_(false), valid_(false) {
    memset(&keys_, 0, sizeof(keys_));
}

PeerStatusSessionCodec::PeerStatusSessionCodec(const PeerDirectionalKeys& keys,
                                               uint8_t localDeviceId, uint8_t peerDeviceId,
                                               uint32_t localBootId, uint32_t peerBootId,
                                               bool localIsInitiator)
    : PeerStatusSessionCodec() {
    reset(keys, localDeviceId, peerDeviceId, localBootId, peerBootId,
          localIsInitiator);
}

PeerStatusSessionCodec::~PeerStatusSessionCodec() { clear(); }

void PeerStatusSessionCodec::reset(const PeerDirectionalKeys& keys,
                                   uint8_t localDeviceId, uint8_t peerDeviceId,
                                   uint32_t localBootId, uint32_t peerBootId,
                                   bool localIsInitiator) {
    clear();
    localDeviceId_ = localDeviceId;
    peerDeviceId_ = peerDeviceId;
    localBootId_ = localBootId;
    peerBootId_ = peerBootId;
    localIsInitiator_ = localIsInitiator;
    memcpy(&keys_, &keys, sizeof(keys_));
    valid_ = localDeviceId_ != 0U && peerDeviceId_ != 0U &&
             localDeviceId_ != peerDeviceId_ && localBootId_ != 0U &&
             peerBootId_ != 0U;
    if (!valid_) clear();
}

void PeerStatusSessionCodec::clear() {
    clearPeerDirectionalKeys(keys_);
    localDeviceId_ = 0U;
    peerDeviceId_ = 0U;
    localBootId_ = 0U;
    peerBootId_ = 0U;
    localIsInitiator_ = false;
    valid_ = false;
}

bool PeerStatusSessionCodec::isValid() const { return valid_; }

bool PeerStatusSessionCodec::encodeOutgoing(const PeerStatusFrame& frame,
                                            uint8_t* output, size_t capacity,
                                            size_t& written) const {
    written = 0U;
    if (!valid_ || output == nullptr || capacity < PEER_STATUS_AUTH_WIRE_SIZE ||
        frame.deviceId != localDeviceId_ || frame.bootId != localBootId_ ||
        !isPeerStatusFrameValid(frame, localDeviceId_)) return false;

    size_t payloadWritten = 0U;
    if (!encodePeerStatusFrame(frame, output, PEER_STATUS_WIRE_SIZE, payloadWritten) ||
        payloadWritten != PEER_STATUS_WIRE_SIZE) return false;

    const uint8_t* key = localIsInitiator_ ? keys_.initiatorToResponder
                                           : keys_.responderToInitiator;
    hmacSha256(key, PEER_SESSION_KEY_SIZE, output, PEER_STATUS_WIRE_SIZE,
               output + PEER_STATUS_WIRE_SIZE);
    written = PEER_STATUS_AUTH_WIRE_SIZE;
    return true;
}

bool PeerStatusSessionCodec::decodeIncoming(const uint8_t* data, size_t length,
                                            PeerStatusFrame& frame) const {
    memset(&frame, 0, sizeof(frame));
    if (!valid_ || data == nullptr || length != PEER_STATUS_AUTH_WIRE_SIZE)
        return false;

    const uint8_t* key = localIsInitiator_ ? keys_.responderToInitiator
                                           : keys_.initiatorToResponder;
    uint8_t expected[HMAC_SHA256_SIZE];
    hmacSha256(key, PEER_SESSION_KEY_SIZE, data, PEER_STATUS_WIRE_SIZE, expected);
    const bool authenticated = constantTimeTagEquals(expected,
                                                       data + PEER_STATUS_WIRE_SIZE);
    memset(expected, 0, sizeof(expected));
    if (!authenticated) return false;

    PeerStatusFrame decoded = {};
    if (!decodePeerStatusFrame(data, PEER_STATUS_WIRE_SIZE, peerDeviceId_, decoded) ||
        decoded.bootId != peerBootId_) return false;
    frame = decoded;
    return true;
}

} // namespace HWC
