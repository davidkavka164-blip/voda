#ifndef HWC_PEER_REQUEST_TRACKER_H
#define HWC_PEER_REQUEST_TRACKER_H

#include "PeerCommandResponse.h"
#include <stddef.h>
#include <stdint.h>

namespace HWC {

// Fixed-size correlation table for locally-originated COMMAND frames.
// It tracks authenticated request/response correlation only; it does not
// transmit, retry, execute, or prove physical water flow.
enum PeerRequestResult {
    PEER_REQUEST_TRACKED,
    PEER_REQUEST_RESPONSE_MATCHED,
    PEER_REQUEST_RESPONSE_UNMATCHED,
    PEER_REQUEST_RESPONSE_MISMATCHED,
    PEER_REQUEST_TIMED_OUT,
    PEER_REQUEST_TABLE_FULL,
    PEER_REQUEST_INVALID
};

class PeerRequestTracker {
public:
    static const size_t CAPACITY = 8U;
    static const size_t RECENT_ID_CAPACITY = 16U;
    // Keep all supported intervals below half the uint32_t millis() range.
    static const uint32_t MAX_TIMEOUT_MS = 0x7FFFFFFFUL;

    PeerRequestTracker();
    void reset();
    // Drops pending requests on session close but preserves recent IDs so a
    // delayed response cannot match a reused ID after a same-boot reconnect.
    void cancelPendingPreserveHistory();

    PeerRequestResult track(const PeerCommandFrame& request,
                            uint32_t nowMs, uint32_t timeoutMs);

    // Call only with a response already authenticated and decoded by the
    // active PeerSessionCodec. A mismatched response never consumes a slot.
    PeerRequestResult acceptResponse(const PeerCommandFrame& response,
                                     uint32_t nowMs,
                                     PeerCommandResponse& parsedResponse);

    // Removes and reports one expired request per call. requestId is zeroed
    // when no request expired.
    bool takeExpired(uint32_t nowMs, uint32_t& requestId);
    size_t pendingCount() const;

private:
    struct Slot {
        PeerCommandFrame request;
        uint32_t startedAtMs;
        uint32_t timeoutMs;
        bool used;
    };
    Slot slots_[CAPACITY];
    uint32_t recentRequestIds_[RECENT_ID_CAPACITY];
    size_t recentNext_;
    size_t recentCount_;
    bool isRecent(uint32_t requestId) const;
    void remember(uint32_t requestId);
    static bool validRequest(const PeerCommandFrame& request);
    static bool expired(const Slot& slot, uint32_t nowMs);
};

} // namespace HWC
#endif
