#ifndef HWC_PEER_STATUS_HMAC_CODEC_H
#define HWC_PEER_STATUS_HMAC_CODEC_H

#include "PeerProtocol.h"
#include <stddef.h>
#include <stdint.h>

namespace HWC {

static const size_t PEER_STATUS_HMAC_KEY_SIZE = 32U;
static const size_t PEER_STATUS_HMAC_TAG_SIZE = 32U;
static const size_t PEER_STATUS_HMAC_WIRE_SIZE =
    PEER_STATUS_WIRE_SIZE + PEER_STATUS_HMAC_TAG_SIZE;

// Stateless authentication for the one-way ESP32 #2 -> ESP32 #1 status feed.
// The caller loads the same 32-byte shared key on both devices.
// HMAC covers the exact existing 52-byte wire frame; no session/handshake state.
bool encodePeerStatusHmacFrame(const PeerStatusFrame& frame,
                               uint8_t expectedDeviceId,
                               const uint8_t* sharedKey, size_t keyLength,
                               uint8_t* output, size_t capacity,
                               size_t& written);

// Authentication is checked before CRC decoding and before the caller may
// pass the decoded frame to PeerStatusMonitor.
bool decodePeerStatusHmacFrame(const uint8_t* data, size_t length,
                               uint8_t expectedDeviceId,
                               const uint8_t* sharedKey, size_t keyLength,
                               PeerStatusFrame& frame);

} // namespace HWC
#endif
