#include "PeerHandshakeWire.h"
#include <string.h>

namespace HWC { namespace {
static const uint8_t MAGIC[4] = {'H', 'W', 'H', '1'};
static const uint8_t VERSION = 2U;

void put32(uint8_t* p, uint32_t v) {
    p[0] = static_cast<uint8_t>(v);
    p[1] = static_cast<uint8_t>(v >> 8U);
    p[2] = static_cast<uint8_t>(v >> 16U);
    p[3] = static_cast<uint8_t>(v >> 24U);
}
uint32_t get32(const uint8_t* p) {
    return static_cast<uint32_t>(p[0]) |
           (static_cast<uint32_t>(p[1]) << 8U) |
           (static_cast<uint32_t>(p[2]) << 16U) |
           (static_cast<uint32_t>(p[3]) << 24U);
}
void header(uint8_t* out, uint8_t type) {
    memcpy(out, MAGIC, sizeof(MAGIC));
    out[4] = VERSION;
    out[5] = type;
}
bool validHeader(const uint8_t* data, size_t length, size_t expected, uint8_t type) {
    return data != 0 && length == expected && memcmp(data, MAGIC, sizeof(MAGIC)) == 0 &&
           data[4] == VERSION && data[5] == type;
}
bool nonzero(const uint8_t* data, size_t length) {
    uint8_t aggregate = 0U;
    for (size_t i = 0; i < length; ++i) aggregate |= data[i];
    return aggregate != 0U;
}
bool validHello(const PeerHandshakeHello& m) {
    return m.initiatorDeviceId != 0U && m.responderDeviceId != 0U &&
           m.initiatorDeviceId != m.responderDeviceId && m.initiatorBootId != 0U &&
           nonzero(m.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
}
bool validChallenge(const PeerHandshakeChallenge& m) {
    return m.initiatorDeviceId != 0U && m.responderDeviceId != 0U &&
           m.initiatorDeviceId != m.responderDeviceId && m.initiatorBootId != 0U &&
           m.responderBootId != 0U && m.authenticatedReplayFloor < 0x80000000UL &&
           nonzero(m.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE) &&
           nonzero(m.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) &&
           memcmp(m.initiatorNonce, m.responderNonce, PEER_HANDSHAKE_NONCE_SIZE) != 0;
}
} // namespace

bool encodePeerHandshakeHello(const PeerHandshakeHello& m, uint8_t out[PEER_HANDSHAKE_HELLO_WIRE_SIZE]) {
    if (out == 0) return false;
    memset(out, 0, PEER_HANDSHAKE_HELLO_WIRE_SIZE);
    if (!validHello(m)) return false;
    header(out, PEER_HANDSHAKE_MESSAGE_HELLO);
    out[6] = m.initiatorDeviceId;
    out[7] = m.responderDeviceId;
    put32(out + 8U, m.initiatorBootId);
    memcpy(out + 12U, m.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
    return true;
}

bool decodePeerHandshakeHello(const uint8_t* data, size_t length, PeerHandshakeHello& out) {
    memset(&out, 0, sizeof(out));
    if (!validHeader(data, length, PEER_HANDSHAKE_HELLO_WIRE_SIZE, PEER_HANDSHAKE_MESSAGE_HELLO)) return false;
    PeerHandshakeHello candidate = {};
    candidate.initiatorDeviceId = data[6];
    candidate.responderDeviceId = data[7];
    candidate.initiatorBootId = get32(data + 8U);
    memcpy(candidate.initiatorNonce, data + 12U, PEER_HANDSHAKE_NONCE_SIZE);
    if (!validHello(candidate)) return false;
    out = candidate;
    return true;
}

bool encodePeerHandshakeChallenge(const PeerHandshakeChallenge& m, uint8_t out[PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE]) {
    if (out == 0) return false;
    memset(out, 0, PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE);
    if (!validChallenge(m)) return false;
    header(out, PEER_HANDSHAKE_MESSAGE_CHALLENGE);
    out[6] = m.initiatorDeviceId;
    out[7] = m.responderDeviceId;
    put32(out + 8U, m.initiatorBootId);
    put32(out + 12U, m.responderBootId);
    put32(out + 16U, m.authenticatedReplayFloor);
    memcpy(out + 20U, m.initiatorNonce, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(out + 36U, m.responderNonce, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(out + 52U, m.tag, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}

bool decodePeerHandshakeChallenge(const uint8_t* data, size_t length, PeerHandshakeChallenge& out) {
    memset(&out, 0, sizeof(out));
    if (!validHeader(data, length, PEER_HANDSHAKE_CHALLENGE_WIRE_SIZE, PEER_HANDSHAKE_MESSAGE_CHALLENGE)) return false;
    PeerHandshakeChallenge candidate = {};
    candidate.initiatorDeviceId = data[6];
    candidate.responderDeviceId = data[7];
    candidate.initiatorBootId = get32(data + 8U);
    candidate.responderBootId = get32(data + 12U);
    candidate.authenticatedReplayFloor = get32(data + 16U);
    memcpy(candidate.initiatorNonce, data + 20U, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(candidate.responderNonce, data + 36U, PEER_HANDSHAKE_NONCE_SIZE);
    memcpy(candidate.tag, data + 52U, PEER_HANDSHAKE_TAG_SIZE);
    if (!validChallenge(candidate)) return false;
    out = candidate;
    return true;
}

bool encodePeerHandshakeFinish(const PeerHandshakeFinish& m, uint8_t out[PEER_HANDSHAKE_FINISH_WIRE_SIZE]) {
    if (out == 0) return false;
    memset(out, 0, PEER_HANDSHAKE_FINISH_WIRE_SIZE);
    header(out, PEER_HANDSHAKE_MESSAGE_FINISH);
    memcpy(out + 6U, m.tag, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}

bool decodePeerHandshakeFinish(const uint8_t* data, size_t length, PeerHandshakeFinish& out) {
    memset(&out, 0, sizeof(out));
    if (!validHeader(data, length, PEER_HANDSHAKE_FINISH_WIRE_SIZE, PEER_HANDSHAKE_MESSAGE_FINISH)) return false;
    memcpy(out.tag, data + 6U, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}


bool encodePeerHandshakeReady(const PeerHandshakeReady& message,
                              uint8_t out[PEER_HANDSHAKE_READY_WIRE_SIZE]) {
    if (out == 0) return false;
    memset(out, 0, PEER_HANDSHAKE_READY_WIRE_SIZE);
    header(out, PEER_HANDSHAKE_MESSAGE_READY);
    memcpy(out + 6U, message.tag, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}

bool decodePeerHandshakeReady(const uint8_t* data, size_t length,
                              PeerHandshakeReady& out) {
    memset(&out, 0, sizeof(out));
    if (!validHeader(data, length, PEER_HANDSHAKE_READY_WIRE_SIZE,
                     PEER_HANDSHAKE_MESSAGE_READY)) return false;
    memcpy(out.tag, data + 6U, PEER_HANDSHAKE_TAG_SIZE);
    return true;
}

} // namespace HWC
