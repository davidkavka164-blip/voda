#include "PeerRequestTracker.h"
#include <string.h>

namespace HWC {

PeerRequestTracker::PeerRequestTracker() { reset(); }

void PeerRequestTracker::reset() {
    memset(slots_, 0, sizeof(slots_));
    memset(recentRequestIds_, 0, sizeof(recentRequestIds_));
    recentNext_ = 0U;
    recentCount_ = 0U;
}

void PeerRequestTracker::cancelPendingPreserveHistory() {
    memset(slots_, 0, sizeof(slots_));
}

bool PeerRequestTracker::isRecent(uint32_t requestId) const {
    for (size_t i = 0; i < recentCount_; ++i)
        if (recentRequestIds_[i] == requestId) return true;
    return false;
}

void PeerRequestTracker::remember(uint32_t requestId) {
    if (requestId == 0U || isRecent(requestId)) return;
    recentRequestIds_[recentNext_] = requestId;
    recentNext_ = (recentNext_ + 1U) % RECENT_ID_CAPACITY;
    if (recentCount_ < RECENT_ID_CAPACITY) ++recentCount_;
}

bool PeerRequestTracker::validRequest(const PeerCommandFrame& request) {
    return request.frameType == PEER_FRAME_COMMAND && request.commandCode != 0U &&
           request.senderDeviceId != 0U && request.targetDeviceId != 0U &&
           request.senderDeviceId != request.targetDeviceId &&
           request.senderBootId != 0U && request.targetBootId != 0U &&
           request.sequence != 0U && request.requestId != 0U;
}

bool PeerRequestTracker::expired(const Slot& slot, uint32_t nowMs) {
    return slot.used && static_cast<uint32_t>(nowMs - slot.startedAtMs) >= slot.timeoutMs;
}

PeerRequestResult PeerRequestTracker::track(const PeerCommandFrame& request,
                                            uint32_t nowMs, uint32_t timeoutMs) {
    if (!validRequest(request) || timeoutMs == 0U || timeoutMs > MAX_TIMEOUT_MS)
        return PEER_REQUEST_INVALID;

    if (isRecent(request.requestId)) return PEER_REQUEST_INVALID;

    size_t freeIndex = CAPACITY;
    for (size_t i = 0; i < CAPACITY; ++i) {
        if (slots_[i].used && slots_[i].request.requestId == request.requestId)
            return PEER_REQUEST_INVALID;
        if (!slots_[i].used && freeIndex == CAPACITY) freeIndex = i;
    }
    if (freeIndex == CAPACITY) return PEER_REQUEST_TABLE_FULL;

    slots_[freeIndex].request = request;
    slots_[freeIndex].startedAtMs = nowMs;
    slots_[freeIndex].timeoutMs = timeoutMs;
    slots_[freeIndex].used = true;
    return PEER_REQUEST_TRACKED;
}

PeerRequestResult PeerRequestTracker::acceptResponse(
    const PeerCommandFrame& response, uint32_t nowMs,
    PeerCommandResponse& parsedResponse) {
    memset(&parsedResponse, 0, sizeof(parsedResponse));
    if (response.frameType != PEER_FRAME_RESPONSE || response.requestId == 0U)
        return PEER_REQUEST_INVALID;

    for (size_t i = 0; i < CAPACITY; ++i) {
        Slot& slot = slots_[i];
        if (!slot.used || slot.request.requestId != response.requestId) continue;
        if (expired(slot, nowMs)) {
            remember(slot.request.requestId);
            slot.used = false;
            memset(&slot.request, 0, sizeof(slot.request));
            return PEER_REQUEST_TIMED_OUT;
        }
        if (!peerCommandResponseMatches(slot.request, response))
            return PEER_REQUEST_RESPONSE_MISMATCHED;
        if (!parsePeerCommandResponse(response, parsedResponse))
            return PEER_REQUEST_RESPONSE_MISMATCHED;
        remember(slot.request.requestId);
        slot.used = false;
        memset(&slot.request, 0, sizeof(slot.request));
        slot.startedAtMs = 0U;
        slot.timeoutMs = 0U;
        return PEER_REQUEST_RESPONSE_MATCHED;
    }
    return PEER_REQUEST_RESPONSE_UNMATCHED;
}

bool PeerRequestTracker::takeExpired(uint32_t nowMs, uint32_t& requestId) {
    requestId = 0U;
    for (size_t i = 0; i < CAPACITY; ++i) {
        Slot& slot = slots_[i];
        if (!expired(slot, nowMs)) continue;
        requestId = slot.request.requestId;
        remember(requestId);
        slot.used = false;
        memset(&slot.request, 0, sizeof(slot.request));
        slot.startedAtMs = 0U;
        slot.timeoutMs = 0U;
        return true;
    }
    return false;
}

size_t PeerRequestTracker::pendingCount() const {
    size_t count = 0U;
    for (size_t i = 0; i < CAPACITY; ++i) if (slots_[i].used) ++count;
    return count;
}

} // namespace HWC
