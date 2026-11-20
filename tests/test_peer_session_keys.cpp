#include "PeerSessionKeys.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main() {
    using namespace HWC;
    uint8_t session[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(session); ++i) session[i] = static_cast<uint8_t>(i + 1U);

    PeerDirectionalKeys initiator = {};
    PeerDirectionalKeys responder = {};
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11223344U, 0x55667788U, initiator));
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11223344U, 0x55667788U, responder));
    // Responder uses the initiator's outbound key to verify inbound traffic,
    // and its own outbound key for traffic back to the initiator.
    assert(memcmp(initiator.initiatorToResponder,
                  responder.initiatorToResponder, PEER_SESSION_KEY_SIZE) == 0);
    assert(memcmp(initiator.responderToInitiator,
                  responder.responderToInitiator, PEER_SESSION_KEY_SIZE) == 0);
    assert(memcmp(initiator.initiatorToResponder,
                  initiator.responderToInitiator, PEER_SESSION_KEY_SIZE) != 0);

    PeerDirectionalKeys changed = {};
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11223345U, 0x55667788U, changed));
    assert(memcmp(initiator.initiatorToResponder,
                  changed.initiatorToResponder, PEER_SESSION_KEY_SIZE) != 0);
    assert(derivePeerDirectionalKeys(session, sizeof(session), 2U, 1U,
                                     0x55667788U, 0x11223344U, changed));
    assert(memcmp(initiator.initiatorToResponder,
                  changed.initiatorToResponder, PEER_SESSION_KEY_SIZE) != 0);

    memset(&changed, 0xA5, sizeof(changed));
    assert(!derivePeerDirectionalKeys(session, 16U, 1U, 2U,
                                      0x11223344U, 0x55667788U, changed));
    for (size_t i = 0; i < sizeof(changed); ++i)
        assert(reinterpret_cast<const uint8_t*>(&changed)[i] == 0U);
    assert(!derivePeerDirectionalKeys(session, sizeof(session), 1U, 1U,
                                      1U, 2U, changed));
    assert(!derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                      0U, 2U, changed));
    assert(!derivePeerDirectionalKeys(0, sizeof(session), 1U, 2U,
                                      1U, 2U, changed));

    clearPeerDirectionalKeys(initiator);
    clearPeerDirectionalKeys(responder);
    puts("PASS: PeerSessionKeys native tests");
    return 0;
}
