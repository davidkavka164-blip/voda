#include "PeerRequestTracker.h"
#include <assert.h>
#include <string.h>
using namespace HWC;

static PeerCommandFrame makeRequest(uint32_t id, uint32_t seq) {
    PeerCommandFrame f; memset(&f, 0, sizeof(f));
    f.frameType = PEER_FRAME_COMMAND; f.commandCode = 4;
    f.senderDeviceId = 1; f.targetDeviceId = 2;
    f.senderBootId = 101; f.targetBootId = 202;
    f.sequence = seq; f.requestId = id; f.argument0 = 500;
    return f;
}

int main() {
    PeerRequestTracker tracker;
    PeerCommandResponse parsed;
    uint32_t expiredId = 999U;
    PeerCommandFrame request = makeRequest(1001U, 7U);
    assert(tracker.track(request, 100U, 500U) == PEER_REQUEST_TRACKED);
    assert(tracker.pendingCount() == 1U);
    assert(tracker.track(request, 101U, 500U) == PEER_REQUEST_INVALID);

    PeerCommandFrame response;
    assert(buildPeerCommandResponse(request, 2U, 202U, 8U,
                                    PEER_RESPONSE_ACK, PEER_REASON_NONE, response));
    response.senderBootId++;
    assert(tracker.acceptResponse(response, 200U, parsed) == PEER_REQUEST_RESPONSE_MISMATCHED);
    assert(tracker.pendingCount() == 1U);
    response.senderBootId--;
    assert(tracker.acceptResponse(response, 200U, parsed) == PEER_REQUEST_RESPONSE_MATCHED);
    assert(parsed.code == PEER_RESPONSE_ACK && tracker.pendingCount() == 0U);
    assert(tracker.acceptResponse(response, 201U, parsed) == PEER_REQUEST_RESPONSE_UNMATCHED);
    // Recently completed IDs are not reusable while delayed packets may exist.
    assert(tracker.track(request, 202U, 500U) == PEER_REQUEST_INVALID);

    PeerCommandFrame r2 = makeRequest(1002U, 9U);
    assert(tracker.track(r2, 0xFFFFFF00UL, 300U) == PEER_REQUEST_TRACKED);
    assert(!tracker.takeExpired(0x00000020UL, expiredId));
    assert(tracker.takeExpired(0x0000002CUL, expiredId));
    assert(expiredId == 1002U && tracker.pendingCount() == 0U);

    PeerCommandFrame r3 = makeRequest(1003U, 10U);
    assert(tracker.track(r3, 1000U, 50U) == PEER_REQUEST_TRACKED);
    PeerCommandFrame lateResponse;
    assert(buildPeerCommandResponse(r3, 2U, 202U, 11U, PEER_RESPONSE_ACK,
                                    PEER_REASON_NONE, lateResponse));
    assert(tracker.acceptResponse(lateResponse, 1050U, parsed) == PEER_REQUEST_TIMED_OUT);
    assert(tracker.pendingCount() == 0U);

    assert(tracker.track(makeRequest(2000U, 20U), 0U, 1U) == PEER_REQUEST_TRACKED);
    for (uint32_t i = 1U; i < PeerRequestTracker::CAPACITY; ++i)
        assert(tracker.track(makeRequest(2000U + i, 20U + i), 0U, 1U) == PEER_REQUEST_TRACKED);
    assert(tracker.track(makeRequest(3000U, 30U), 0U, 1U) == PEER_REQUEST_TABLE_FULL);
    assert(tracker.track(makeRequest(3001U, 31U), 0U, 0U) == PEER_REQUEST_INVALID);
    assert(tracker.track(makeRequest(3002U, 32U), 0U,
                         PeerRequestTracker::MAX_TIMEOUT_MS + 1U) == PEER_REQUEST_INVALID);
    PeerCommandFrame invalid = makeRequest(4000U, 40U); invalid.frameType = PEER_FRAME_RESPONSE;
    assert(tracker.track(invalid, 0U, 10U) == PEER_REQUEST_INVALID);
    tracker.cancelPendingPreserveHistory();
    assert(tracker.pendingCount() == 0U);
    assert(tracker.track(makeRequest(1001U, 99U), 0U, 10U) == PEER_REQUEST_INVALID);
    tracker.reset();
    assert(tracker.pendingCount() == 0U);
    assert(tracker.track(makeRequest(1001U, 1U), 0U, 10U) == PEER_REQUEST_TRACKED);
    return 0;
}
