#ifndef HWC_PEER_SESSION_KEYS_H
#define HWC_PEER_SESSION_KEYS_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

static const size_t PEER_SESSION_KEY_SIZE = 32U;

struct PeerDirectionalKeys {
    uint8_t initiatorToResponder[PEER_SESSION_KEY_SIZE];
    uint8_t responderToInitiator[PEER_SESSION_KEY_SIZE];
};

// Derives independent direction-specific HMAC keys from the authenticated
// handshake session key. Device IDs and both boot IDs are bound to the KDF.
// Both peers MUST use the same initiator/responder ordering from the handshake.
bool derivePeerDirectionalKeys(const uint8_t* sessionKey, size_t sessionKeyLength,
                               uint8_t initiatorDeviceId, uint8_t responderDeviceId,
                               uint32_t initiatorBootId, uint32_t responderBootId,
                               PeerDirectionalKeys& keys);
void clearPeerDirectionalKeys(PeerDirectionalKeys& keys);

} // namespace HWC
#endif
