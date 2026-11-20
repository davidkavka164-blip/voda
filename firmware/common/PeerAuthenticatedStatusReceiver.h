#ifndef HWC_PEER_AUTHENTICATED_STATUS_RECEIVER_H
#define HWC_PEER_AUTHENTICATED_STATUS_RECEIVER_H

#include "PeerProtocol.h"
#include "PeerStatusSessionCodec.h"

namespace HWC {

enum PeerAuthenticatedStatusResult : uint8_t {
    PEER_AUTH_STATUS_ACCEPTED = 0U,
    PEER_AUTH_STATUS_NO_SESSION = 1U,
    PEER_AUTH_STATUS_REJECT_AUTHENTICATION = 2U,
    PEER_AUTH_STATUS_REJECT_REPLAY_OR_STALE = 3U
};

// Security boundary for the receiver path. It authenticates the complete
// status frame before publishing it to PeerStatusMonitor. The caller owns the
// codec and must clear/reset it whenever the peer session ends.
class PeerAuthenticatedStatusReceiver {
public:
    explicit PeerAuthenticatedStatusReceiver(PeerStatusMonitor& monitor);
    PeerAuthenticatedStatusReceiver(const PeerAuthenticatedStatusReceiver&) = delete;
    PeerAuthenticatedStatusReceiver& operator=(const PeerAuthenticatedStatusReceiver&) = delete;

    void bindSession(PeerStatusSessionCodec& codec);
    void clearSession();
    PeerAuthenticatedStatusResult accept(const uint8_t* data, size_t length,
                                         uint32_t receivedAtMs,
                                         PeerStatusFrame& acceptedFrame);
    bool sessionBound() const;

private:
    PeerStatusMonitor& monitor_;
    PeerStatusSessionCodec* codec_;
};

} // namespace HWC
#endif
