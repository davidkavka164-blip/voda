#ifndef HWC_PEER_SESSION_CODEC_H
#define HWC_PEER_SESSION_CODEC_H

#include "PeerCommandFrame.h"
#include "PeerSessionKeys.h"

namespace HWC {

// Binds authenticated command frames to one established handshake session.
// The caller must only construct this context after successful FINISH.
// No transport or session establishment is performed by this class.
class PeerSessionCodec {
public:
    PeerSessionCodec();
    PeerSessionCodec(const PeerDirectionalKeys& keys,
                     uint8_t localDeviceId, uint8_t peerDeviceId,
                     uint32_t localBootId, uint32_t peerBootId,
                     bool localIsInitiator);

    ~PeerSessionCodec();

    PeerSessionCodec(const PeerSessionCodec&) = delete;
    PeerSessionCodec& operator=(const PeerSessionCodec&) = delete;

    bool isValid() const;
    void reset(const PeerDirectionalKeys& keys,
               uint8_t localDeviceId, uint8_t peerDeviceId,
               uint32_t localBootId, uint32_t peerBootId,
               bool localIsInitiator);
    void clear();

    bool encodeOutgoing(const PeerCommandFrame& frame,
                        uint8_t* output, size_t capacity, size_t& written) const;
    bool decodeIncoming(const uint8_t* data, size_t length,
                        PeerCommandFrame& frame) const;

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
