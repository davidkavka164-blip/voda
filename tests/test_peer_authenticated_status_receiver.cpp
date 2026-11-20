#include "PeerAuthenticatedStatusReceiver.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace HWC;

int main() {
    uint8_t session[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(session); ++i)
        session[i] = static_cast<uint8_t>(0x31U + i);

    PeerDirectionalKeys keys;
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11112222UL, 0x33334444UL, keys));
    PeerStatusSessionCodec sender(keys, 2U, 1U, 0x33334444UL,
                                  0x11112222UL, false);
    PeerStatusSessionCodec receiverCodec(keys, 1U, 2U, 0x11112222UL,
                                         0x33334444UL, true);
    PeerStatusMonitor monitor;
    PeerAuthenticatedStatusReceiver receiver(monitor);
    PeerStatusFrame accepted = {};
    uint8_t wire[PEER_STATUS_AUTH_WIRE_SIZE];
    size_t written = 0U;

    PeerStatusFrame frame = makeEmptyPeerStatus(0x33334444UL, 1U, 100U);
    frame.upperLevelValid = 1U;
    frame.upperLevelCm = 75.0f;
    updatePeerStatusChecksum(frame);
    assert(sender.encodeOutgoing(frame, wire, sizeof(wire), written));

    // No bound session: even a valid HMAC packet cannot refresh monitor state.
    assert(receiver.accept(wire, written, 1000U, accepted) == PEER_AUTH_STATUS_NO_SESSION);
    assert(!monitor.hasFrame());

    receiver.bindSession(receiverCodec);
    assert(receiver.sessionBound());
    assert(receiver.accept(wire, written, 1000U, accepted) == PEER_AUTH_STATUS_ACCEPTED);
    assert(monitor.hasFrame());
    assert(accepted.sequence == 1U && accepted.upperLevelCm == 75.0f);

    // Exact replay is rejected after authentication and does not refresh receive time.
    assert(receiver.accept(wire, written, 2000U, accepted) == PEER_AUTH_STATUS_REJECT_REPLAY_OR_STALE);
    assert(accepted.bootId == 0U);
    assert(monitor.ageSinceReceiveMs(2500U) == 1500U);

    // A valid-CRC but unauthenticated alteration cannot reach the monitor.
    PeerStatusFrame next = frame;
    next.sequence = 2U;
    next.upperLevelCm = 74.0f;
    updatePeerStatusChecksum(next);
    assert(sender.encodeOutgoing(next, wire, sizeof(wire), written));
    wire[24] ^= 0x01U;
    assert(receiver.accept(wire, written, 3000U, accepted) == PEER_AUTH_STATUS_REJECT_AUTHENTICATION);
    assert(accepted.bootId == 0U);
    assert(monitor.frame().sequence == 1U);
    assert(monitor.ageSinceReceiveMs(3000U) == 2000U);

    // A cleared codec invalidates the binding immediately, even if the pointer remains.
    receiverCodec.clear();
    assert(!receiver.sessionBound());
    assert(receiver.accept(wire, written, 4000U, accepted) == PEER_AUTH_STATUS_NO_SESSION);
    receiver.clearSession();

    sender.clear();
    clearPeerDirectionalKeys(keys);
    puts("PASS: authenticated status receiver verifies before monitor publication and rejects replay");
    return 0;
}
