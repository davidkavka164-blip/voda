#ifndef HWC_PEER_HANDSHAKE_READY_H
#define HWC_PEER_HANDSHAKE_READY_H

#include "PeerHandshake.h"

namespace HWC {

// Final responder proof. The initiator MUST NOT activate command traffic until
// this proof is verified with the session key derived from the authenticated
// HELLO/CHALLENGE/FINISH transcript.
struct PeerHandshakeReady {
    uint8_t tag[PEER_HANDSHAKE_TAG_SIZE];
};

bool createPeerHandshakeReady(const uint8_t* sessionKey, size_t keyLength,
                              PeerHandshakeReady& ready);
bool verifyPeerHandshakeReady(const uint8_t* sessionKey, size_t keyLength,
                              const PeerHandshakeReady& ready);

} // namespace HWC
#endif
