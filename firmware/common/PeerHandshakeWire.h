#ifndef HWC_PEER_HANDSHAKE_WIRE_H
#define HWC_PEER_HANDSHAKE_WIRE_H

#include <stddef.h>
#include <stdint.h>
#include "PeerHandshake.h"
#include "PeerHandshakeReady.h"

namespace HWC {

static const size_t PEER_HANDSHAKE_HELLO_WIRE_SIZE = 28U;
static const size_t PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE = 84U;
static const size_t PEER_HANDSHAKE_FINISH_WIRE_SIZE = 38U;
static const size_t PEER_HANDSHAKE_READY_WIRE_SIZE = 38U;

enum PeerHandshakeMessageType : uint8_t {
    PEER_HANDSHAKE_MESSAGE_HELLO = 1U,
    PEER_HANDSHAKE_MESSAGE_CHALLENGE = 2U,
    PEER_HANDSHAKE_MESSAGE_FINISH = 3U,
    PEER_HANDSHAKE_MESSAGE_READY = 4U
};

// Canonical fixed-length wire encoding. Never transmit the in-memory structs:
// their padding and alignment are compiler/target dependent.
bool encodePeerHandshakeHello(const PeerHandshakeHello& message,
                              uint8_t out[PEER_HANDSHAKE_HELLO_WIRE_SIZE]);
bool decodePeerHandshakeHello(const uint8_t* data, size_t length,
                              PeerHandshakeHello& out);
bool encodePeerHandshakeChallenge(const PeerHandshakeChallenge& message,
                                  uint8_t out[PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE]);
bool decodePeerHandshakeChallenge(const uint8_t* data, size_t length,
                                  PeerHandshakeChallenge& out);
bool encodePeerHandshakeFinish(const PeerHandshakeFinish& message,
                               uint8_t out[PEER_HANDSHAKE_FINISH_WIRE_SIZE]);
bool decodePeerHandshakeFinish(const uint8_t* data, size_t length,
                               PeerHandshakeFinish& out);
bool encodePeerHandshakeReady(const PeerHandshakeReady& message,
                              uint8_t out[PEER_HANDSHAKE_READY_WIRE_SIZE]);
bool decodePeerHandshakeReady(const uint8_t* data, size_t length,
                              PeerHandshakeReady& out);

} // namespace HWC
#endif
