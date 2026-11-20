#include "PeerCommandGuard.h"

namespace HWC {

PeerCommandGuard::PeerCommandGuard() { reset(0U, 0U); }

void PeerCommandGuard::reset(uint8_t localDeviceId, uint32_t localBootId) {
    localDeviceId_ = localDeviceId;
    peerDeviceId_ = 0U;
    localBootId_ = localBootId;
    peerBootId_ = 0U;
    acceptedCount_ = 0U;
    sessionEstablished_ = false;
    clearReplayState();
}

void PeerCommandGuard::endSession() {
    sessionEstablished_ = false;
    peerBootId_ = 0U;
    lastSequence_ = 0U;
    // Keep the recent request-ID cache in RAM across a link/session restart.
}

bool PeerCommandGuard::recordAuthenticatedPeerSession(uint8_t peerDeviceId,
                                                       uint32_t peerBootId,
                                                       bool freshHandshakeAuthenticated) {
    if (!freshHandshakeAuthenticated || localDeviceId_ == 0U || localBootId_ == 0U ||
        peerDeviceId == 0U || peerBootId == 0U || peerDeviceId == localDeviceId_)
        return false;
    if (peerDeviceId_ != 0U && peerDeviceId != peerDeviceId_) return false;
    // A peer boot transition is accepted only through this explicit session API.
    peerDeviceId_ = peerDeviceId;
    peerBootId_ = peerBootId;
    // A fresh authenticated handshake resets sequence ordering, while the
    // recent request-ID cache is retained to catch application-level retries
    // that survive a peer reboot or re-handshake.
    lastSequence_ = 0U;
    sessionEstablished_ = true;
    return true;
}

bool PeerCommandGuard::hasRequestId(uint32_t requestId) const {
    for (size_t i = 0; i < requestCount_; ++i)
        if (requestIds_[i] == requestId) return true;
    return false;
}

void PeerCommandGuard::rememberRequestId(uint32_t requestId) {
    requestIds_[nextRequestIndex_] = requestId;
    nextRequestIndex_ = (nextRequestIndex_ + 1U) % REQUEST_CACHE_SIZE;
    if (requestCount_ < REQUEST_CACHE_SIZE) ++requestCount_;
}

void PeerCommandGuard::clearReplayState() {
    lastSequence_ = 0U;
    nextRequestIndex_ = 0U;
    requestCount_ = 0U;
    for (size_t i = 0; i < REQUEST_CACHE_SIZE; ++i) requestIds_[i] = 0U;
}

PeerCommandAdmission PeerCommandGuard::admit(const PeerCommandMetadata& command,
                                              bool frameAuthenticated) {
    if (!frameAuthenticated) return PEER_COMMAND_REJECT_UNAUTHENTICATED;
    if (localDeviceId_ == 0U || localBootId_ == 0U || command.senderDeviceId == 0U ||
        command.targetDeviceId == 0U || command.senderBootId == 0U ||
        command.senderSequence == 0U || command.requestId == 0U || command.targetBootId == 0U)
        return PEER_COMMAND_REJECT_INVALID_ID;
    if (command.targetDeviceId != localDeviceId_ || command.senderDeviceId != peerDeviceId_)
        return PEER_COMMAND_REJECT_WRONG_DEVICE;
    if (command.targetBootId != localBootId_)
        return PEER_COMMAND_REJECT_WRONG_TARGET_BOOT;
    if (!sessionEstablished_ || command.senderBootId != peerBootId_)
        return PEER_COMMAND_REJECT_UNESTABLISHED_SESSION;
    if (hasRequestId(command.requestId)) return PEER_COMMAND_REJECT_DUPLICATE_REQUEST;
    // Signed subtraction gives wrap-safe ordering for sequence gaps below 2^31.
    if (static_cast<int32_t>(command.senderSequence - lastSequence_) <= 0)
        return PEER_COMMAND_REJECT_REPLAYED_SEQUENCE;
    lastSequence_ = command.senderSequence;
    rememberRequestId(command.requestId);
    ++acceptedCount_;
    return PEER_COMMAND_ACCEPTED;
}

uint32_t PeerCommandGuard::acceptedCount() const { return acceptedCount_; }
uint32_t PeerCommandGuard::peerBootId() const { return peerBootId_; }

} // namespace HWC
