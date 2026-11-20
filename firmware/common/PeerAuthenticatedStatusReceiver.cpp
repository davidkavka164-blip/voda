#include "PeerAuthenticatedStatusReceiver.h"
#include <string.h>

namespace HWC {

PeerAuthenticatedStatusReceiver::PeerAuthenticatedStatusReceiver(PeerStatusMonitor& monitor)
    : monitor_(monitor), codec_(0) {}

void PeerAuthenticatedStatusReceiver::bindSession(PeerStatusSessionCodec& codec) {
    codec_ = codec.isValid() ? &codec : 0;
}

void PeerAuthenticatedStatusReceiver::clearSession() { codec_ = 0; }

bool PeerAuthenticatedStatusReceiver::sessionBound() const {
    return codec_ != 0 && codec_->isValid();
}

PeerAuthenticatedStatusResult PeerAuthenticatedStatusReceiver::accept(
        const uint8_t* data, size_t length, uint32_t receivedAtMs,
        PeerStatusFrame& acceptedFrame) {
    memset(&acceptedFrame, 0, sizeof(acceptedFrame));
    if (!sessionBound()) return PEER_AUTH_STATUS_NO_SESSION;

    PeerStatusFrame candidate = {};
    if (!codec_->decodeIncoming(data, length, candidate))
        return PEER_AUTH_STATUS_REJECT_AUTHENTICATION;

    // This is deliberately the first point at which the decoded frame reaches
    // the monitor. CRC-only or unauthenticated telemetry cannot refresh peer
    // freshness, sequence state, boot epoch, or transfer inputs.
    if (!monitor_.accept(candidate, candidate.deviceId, receivedAtMs))
        return PEER_AUTH_STATUS_REJECT_REPLAY_OR_STALE;

    acceptedFrame = candidate;
    return PEER_AUTH_STATUS_ACCEPTED;
}

} // namespace HWC
