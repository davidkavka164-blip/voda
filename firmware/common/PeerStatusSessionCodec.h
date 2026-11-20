#ifndef HWC_PEER_STATUS_SESSION_CODEC_H
#define HWC_PEER_STATUS_SESSION_CODEC_H

#include "PeerProtocol.h"
#include "PeerSessionKeys.h"

namespace HWC {

// Existing status payload (52 B) followed by a full HMAC-SHA-256 tag (32 B).
// CRC remains useful for accidental corruption diagnostics, but HMAC is the
// security boundary. Never pass decoded data to PeerStatusMonitor before this
// codec has authenticated it.
static const size_t PEER_STATUS_AUTH_TAG_SIZE = 32U;
static const size_t PEER_STATUS_AUTH_WIRE_SIZE =
    PEER_STATUS_WIRE_SIZE + PEER_STATUS_AUTH_TAG_SIZE;

class PeerStatusSessionCodec {
public:
    PeerStatusSessionCodec();
    PeerStatusSessionCodec(const PeerDirectionalKeys& keys,
                           uint8_t localDeviceId, uint8_t peerDeviceId,
                           uint32_t localBootId, uint32_t peerBootId,
                           bool localIsInitiator);
    ~PeerStatusSessionCodec();

    PeerStatusSessionCodec(const PeerStatusSessionCodec&) = delete;
    PeerStatusSessionCodec& operator=(const PeerStatusSessionCodec&) = delete;

    bool isValid() const;
    void reset(const PeerDirectionalKeys& keys,
               uint8_t localDeviceId, uint8_t peerDeviceId,
               uint32_t localBootId, uint32_t peerBootId,
               bool localIsInitiator);
    void clear();

    bool encodeOutgoing(const PeerStatusFrame& frame,
                        uint8_t* output, size_t capacity, size_t& written) const;
    bool decodeIncoming(const uint8_t* data, size_t length,
                        PeerStatusFrame& frame) const;

private:
    PeerDirectionalKeys keys_;
    uint8_t localDeviceId_;
    uint8_t peerDeviceId_;
    uint32_t localBootId_;
    uint32_t peerBootId_;
    bool localIsInitiator_;
    bool valid_;
};

} // namespace HWC
#endif
