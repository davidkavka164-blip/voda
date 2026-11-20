#include "PeerCommandGuard.h"
#include <assert.h>
#include <stdio.h>

int main() {
    using namespace HWC;
    PeerCommandGuard guard;
    guard.reset(1U, 0xAABBCCDDU);
    PeerCommandMetadata c = { 2U, 1U, 0x11223344U, 1U, 100U, 0xAABBCCDDU };

    assert(guard.admit(c, false) == PEER_COMMAND_REJECT_UNAUTHENTICATED);
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_DEVICE); // no session/peer pin
    assert(!guard.recordAuthenticatedPeerSession(2U, 0x11223344U, false));
    assert(!guard.recordAuthenticatedPeerSession(1U, 0x11223344U, true)); // cannot pair to self
    assert(guard.recordAuthenticatedPeerSession(2U, 0x11223344U, true));
    assert(guard.peerBootId() == 0x11223344U);

    assert(guard.admit(c, true) == PEER_COMMAND_ACCEPTED);
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_DUPLICATE_REQUEST);
    c.requestId = 101U;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_REPLAYED_SEQUENCE);
    c.senderSequence = 2U;
    assert(guard.admit(c, true) == PEER_COMMAND_ACCEPTED);

    c.requestId = 102U;
    c.targetDeviceId = 3U;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_DEVICE);
    c.targetDeviceId = 1U;
    c.targetBootId++;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_TARGET_BOOT);
    c.targetBootId--;
    c.senderDeviceId = 3U;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_DEVICE);
    c.senderDeviceId = 2U;
    c.senderBootId = 0x55667788U;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_UNESTABLISHED_SESSION);

    // A peer reboot cannot silently reset the sequence space. It requires a
    // separately verified fresh handshake and then uses a new boot ID.
    assert(guard.recordAuthenticatedPeerSession(2U, 0x55667788U, true));
    c.senderBootId = 0x55667788U;
    c.senderSequence = 1U;
    c.requestId = 103U;
    assert(guard.admit(c, true) == PEER_COMMAND_ACCEPTED);
    c.requestId = 100U;
    c.senderSequence = 2U;
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_DUPLICATE_REQUEST);
    assert(guard.acceptedCount() == 3U);

    // Local reboot changes target boot ID and clears volatile replay state;
    // old captured frames are still rejected because they target the old boot.
    guard.reset(1U, 0xAABBCCDEU);
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_DEVICE);
    assert(guard.recordAuthenticatedPeerSession(2U, 0x55667788U, true));
    assert(guard.admit(c, true) == PEER_COMMAND_REJECT_WRONG_TARGET_BOOT);
    puts("PASS: PeerCommandGuard native tests");
    return 0;
}
