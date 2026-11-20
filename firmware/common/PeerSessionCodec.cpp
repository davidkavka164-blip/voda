#include "PeerSessionCodec.h"
#include <string.h>

namespace HWC {

PeerSessionCodec::PeerSessionCodec()
    : localDeviceId_(0U), peerDeviceId_(0U), localBootId_(0U),
      peerBootId_(0U), localIsInitiator_(false), valid_(false) {
    memset(&keys_, 0, sizeof(keys_));
}

PeerSessionCodec::PeerSessionCodec(const PeerDirectionalKeys& keys,
                                   uint8_t localDeviceId, uint8_t peerDeviceId,
                                   uint32_t localBootId, uint32_t peerBootId,
                                   bool localIsInitiator)
    : PeerSessionCodec() {
    reset(keys, localDeviceId, peerDeviceId, localBootId, peerBootId,
          localIsInitiator);
}

PeerSessionCodec::~PeerSessionCodec() { clear(); }

void PeerSessionCodec::reset(const PeerDirectionalKeys& keys,
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

void PeerSessionCodec::clear() {
    clearPeerDirectionalKeys(keys_);
    localDeviceId_ = 0U;
    peerDeviceId_ = 0U;
    localBootId_ = 0U;
    peerBootId_ = 0U;
    localIsInitiator_ = false;
    valid_ = false;
}

bool PeerSessionCodec::isValid() const { return valid_; }

bool PeerSessionCodec::encodeOutgoing(const PeerCommandFrame& frame,
                                      uint8_t* output, size_t capacity,
                                      size_t& written) const {
    written = 0U;
    if (!valid_ || frame.senderDeviceId != localDeviceId_ ||
        frame.targetDeviceId != peerDeviceId_ ||
        frame.senderBootId != localBootId_ || frame.targetBootId != peerBootId_) {
        return false;
    }
    const uint8_t* key = localIsInitiator_ ? keys_.initiatorToResponder
                                           : keys_.responderToInitiator;
    return encodePeerCommandFrame(frame, key, PEER_SESSION_KEY_SIZE,
                                  output, capacity, written);
}

bool PeerSessionCodec::decodeIncoming(const uint8_t* data, size_t length,
                                      PeerCommandFrame& frame) const {
    memset(&frame, 0, sizeof(frame));
    if (!valid_) return false;
    const uint8_t* key = localIsInitiator_ ? keys_.responderToInitiator
                                           : keys_.initiatorToResponder;
    PeerCommandFrame decoded;
    if (!decodePeerCommandFrame(data, length, key, PEER_SESSION_KEY_SIZE, decoded))
        return false;
    if (decoded.senderDeviceId != peerDeviceId_ ||
        decoded.targetDeviceId != localDeviceId_ ||
        decoded.senderBootId != peerBootId_ || decoded.targetBootId != localBootId_) {
        return false;
    }
    frame = decoded;
    return true;
}

} // namespace HWC
