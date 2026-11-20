#include "PeerCommandSession.h"
#include <string.h>

namespace HWC {

PeerCommandSession::PeerCommandSession()
    : persistentReplayEnabled_(false), active_(false), localDeviceId_(0U), localBootId_(0U) {
    guard_.reset(0U, 0U);
}

bool PeerCommandSession::activate(const PeerDirectionalKeys& keys,
                                  uint8_t localDeviceId, uint8_t peerDeviceId,
                                  uint32_t localBootId, uint32_t peerBootId,
                                  bool localIsInitiator,
                                  bool freshHandshakeAuthenticated) {
    close();
    if (!freshHandshakeAuthenticated) return false;
    if (localDeviceId_ != localDeviceId || localBootId_ != localBootId) {
        guard_.reset(localDeviceId, localBootId);
        localDeviceId_ = localDeviceId;
        localBootId_ = localBootId;
    }

    if (!outgoingSequenceAllocator_.beginBoot(localBootId)) {
        close();
        return false;
    }

    codec_.reset(keys, localDeviceId, peerDeviceId, localBootId, peerBootId,
                 localIsInitiator);
    if (!codec_.isValid()) {
        close();
        return false;
    }

    if (!guard_.recordAuthenticatedPeerSession(peerDeviceId, peerBootId,
                                               freshHandshakeAuthenticated)) {
        close();
        return false;
    }
    active_ = true;
    return true;
}

bool PeerCommandSession::activateWithPersistentReplay(
    const PeerDirectionalKeys& keys, uint8_t localDeviceId, uint8_t peerDeviceId,
    uint32_t localBootId, uint32_t peerBootId, bool localIsInitiator,
    bool freshHandshakeAuthenticated, IPeerReplayStore& replayStore,
    uint32_t reservationWindow) {
    if (!activate(keys, localDeviceId, peerDeviceId, localBootId, peerBootId,
                  localIsInitiator, freshHandshakeAuthenticated)) return false;
    if (!persistentReplayGuard_.beginAuthenticatedSession(
            localDeviceId, peerDeviceId, localBootId, peerBootId,
            replayStore, reservationWindow)) {
        close();
        return false;
    }
    persistentReplayEnabled_ = true;
    return true;
}

void PeerCommandSession::close() {
    active_ = false;
    codec_.clear();
    guard_.endSession();
    persistentReplayGuard_.endSession();
    persistentReplayEnabled_ = false;
    requestTracker_.cancelPendingPreserveHistory();
}

bool PeerCommandSession::isActive() const {
    return active_ && codec_.isValid();
}

bool PeerCommandSession::encodeOutgoing(const PeerCommandFrame& frame,
                                        uint8_t* output, size_t capacity,
                                        size_t& written) const {
    written = 0U;
    if (!isActive()) return false;
    return codec_.encodeOutgoing(frame, output, capacity, written);
}

PeerRequestResult PeerCommandSession::prepareOutgoingRequest(
    const PeerCommandFrame& request, uint32_t nowMs, uint32_t timeoutMs,
    uint8_t* output, size_t capacity, size_t& written) {
    written = 0U;
    if (!isActive() || output == 0) return PEER_REQUEST_INVALID;
    if (request.frameType != PEER_FRAME_COMMAND) return PEER_REQUEST_INVALID;

    PeerCommandFrame sequencedRequest = request;
    if (!outgoingSequenceAllocator_.allocate(sequencedRequest.sequence))
        return PEER_REQUEST_INVALID;

    size_t encodedLength = 0U;
    if (!codec_.encodeOutgoing(sequencedRequest, output, capacity, encodedLength))
        return PEER_REQUEST_INVALID;

    const PeerRequestResult result = requestTracker_.track(sequencedRequest, nowMs, timeoutMs);
    if (result != PEER_REQUEST_TRACKED) {
        memset(output, 0, encodedLength);
        return result;
    }
    written = encodedLength;
    return PEER_REQUEST_TRACKED;
}

PeerRequestResult PeerCommandSession::receiveResponse(
    const uint8_t* data, size_t length, uint32_t nowMs,
    PeerCommandResponse& response) {
    memset(&response, 0, sizeof(response));
    if (!isActive()) return PEER_REQUEST_INVALID;

    PeerCommandFrame decoded;
    memset(&decoded, 0, sizeof(decoded));
    if (!codec_.decodeIncoming(data, length, decoded) ||
        decoded.frameType != PEER_FRAME_RESPONSE)
        return PEER_REQUEST_INVALID;

    return requestTracker_.acceptResponse(decoded, nowMs, response);
}

bool PeerCommandSession::takeExpiredRequest(uint32_t nowMs, uint32_t& requestId) {
    if (!isActive()) {
        requestId = 0U;
        return false;
    }
    return requestTracker_.takeExpired(nowMs, requestId);
}

size_t PeerCommandSession::pendingRequestCount() const {
    return requestTracker_.pendingCount();
}

uint32_t PeerCommandSession::persistentReplayFloor() const {
    return persistentReplayEnabled_ ? persistentReplayGuard_.reservedThrough() : 0U;
}

bool PeerCommandSession::synchronizeOutgoingSequenceFloor(
    uint32_t authenticatedReservedFloor) {
    if (!isActive()) return false;
    return outgoingSequenceAllocator_.advancePastAuthenticatedFloor(
        authenticatedReservedFloor);
}

uint32_t PeerCommandSession::nextOutgoingSequence() const {
    return outgoingSequenceAllocator_.nextSequence();
}

PeerCommandAdmission PeerCommandSession::receiveCommand(
    const uint8_t* data, size_t length, PeerCommandFrame& acceptedCommand) {
    memset(&acceptedCommand, 0, sizeof(acceptedCommand));
    if (!isActive()) return PEER_COMMAND_REJECT_UNESTABLISHED_SESSION;

    PeerCommandFrame decoded;
    if (!codec_.decodeIncoming(data, length, decoded))
        return PEER_COMMAND_REJECT_UNAUTHENTICATED;
    if (decoded.frameType != PEER_FRAME_COMMAND)
        return PEER_COMMAND_REJECT_WRONG_FRAME_TYPE;

    PeerCommandMetadata metadata;
    metadata.senderDeviceId = decoded.senderDeviceId;
    metadata.targetDeviceId = decoded.targetDeviceId;
    metadata.senderBootId = decoded.senderBootId;
    metadata.senderSequence = decoded.sequence;
    metadata.requestId = decoded.requestId;
    metadata.targetBootId = decoded.targetBootId;

    if (persistentReplayEnabled_) {
        // Persist/advance the sequence reservation first. If the volatile guard
        // then rejects a duplicate request ID, consuming that sequence is safe:
        // it must never become admissible after a restart.
        const PeerCommandAdmission persistentAdmission =
            persistentReplayGuard_.admit(metadata, true);
        if (persistentAdmission != PEER_COMMAND_ACCEPTED) return persistentAdmission;
    }
    const PeerCommandAdmission admission = guard_.admit(metadata, true);
    if (admission != PEER_COMMAND_ACCEPTED) return admission;
    acceptedCommand = decoded;
    return PEER_COMMAND_ACCEPTED;
}

uint32_t PeerCommandSession::acceptedCommandCount() const {
    return guard_.acceptedCount();
}

uint32_t PeerCommandSession::peerBootId() const {
    return guard_.peerBootId();
}

} // namespace HWC
