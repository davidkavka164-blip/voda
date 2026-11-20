#include "PeerStatusSessionCodec.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

using namespace HWC;

int main() {
    uint8_t session[PEER_SESSION_KEY_SIZE];
    for (size_t i = 0; i < sizeof(session); ++i)
        session[i] = static_cast<uint8_t>(0xA0U + i);

    PeerDirectionalKeys keys;
    assert(derivePeerDirectionalKeys(session, sizeof(session), 1U, 2U,
                                     0x11223344UL, 0x55667788UL, keys));

    // ESP32 #2 is the responder and sends status to ESP32 #1 (initiator).
    PeerStatusSessionCodec sender(keys, 2U, 1U, 0x55667788UL,
                                  0x11223344UL, false);
    PeerStatusSessionCodec receiver(keys, 1U, 2U, 0x11223344UL,
                                    0x55667788UL, true);
    assert(sender.isValid() && receiver.isValid());

    PeerStatusFrame status = makeEmptyPeerStatus(0x55667788UL, 7U, 123456U);
    status.upperLevelValid = 1U;
    status.upperLevelCm = 87.5f;
    status.upperVolumeValid = 1U;
    status.upperVolumeLiters = 3210.0f;
    status.temperatureValid = 1U;
    status.temperatureC = 18.25f;
    status.pumpProtectionReady = 1U;
    status.upperLimitsChecksum = 0x1234ABCDUL;
    status.alarmBits = 0x20U;
    updatePeerStatusChecksum(status);

    uint8_t wire[PEER_STATUS_AUTH_WIRE_SIZE];
    size_t written = 0U;
    assert(sender.encodeOutgoing(status, wire, sizeof(wire), written));
    assert(written == PEER_STATUS_AUTH_WIRE_SIZE);

    PeerStatusFrame decoded = {};
    assert(receiver.decodeIncoming(wire, written, decoded));
    assert(decoded.sequence == status.sequence);
    assert(decoded.bootId == status.bootId);
    assert(decoded.upperLevelCm == status.upperLevelCm);
    assert(decoded.upperVolumeLiters == status.upperVolumeLiters);
    assert(decoded.temperatureC == status.temperatureC);
    assert(decoded.alarmBits == status.alarmBits);

    // Authentication rejects payload tampering, tag tampering and truncation.
    uint8_t altered[PEER_STATUS_AUTH_WIRE_SIZE];
    memcpy(altered, wire, sizeof(altered));
    altered[24] ^= 0x01U;
    assert(!receiver.decodeIncoming(altered, sizeof(altered), decoded));
    memcpy(altered, wire, sizeof(altered));
    altered[PEER_STATUS_WIRE_SIZE + 7U] ^= 0x80U;
    assert(!receiver.decodeIncoming(altered, sizeof(altered), decoded));
    assert(!receiver.decodeIncoming(wire, PEER_STATUS_WIRE_SIZE, decoded));

    // Session identity and direction are pinned to both device boot epochs.
    PeerStatusSessionCodec wrongBoot(keys, 1U, 2U, 0x11223344UL,
                                     0x55667789UL, true);
    PeerStatusSessionCodec wrongRole(keys, 1U, 2U, 0x11223344UL,
                                     0x55667788UL, false);
    assert(!wrongBoot.decodeIncoming(wire, written, decoded));
    assert(!wrongRole.decodeIncoming(wire, written, decoded));

    // Sender cannot authenticate a status frame for a different device/boot.
    PeerStatusFrame forged = status;
    forged.deviceId = 1U;
    updatePeerStatusChecksum(forged);
    assert(!sender.encodeOutgoing(forged, wire, sizeof(wire), written));
    forged = status;
    forged.bootId++;
    updatePeerStatusChecksum(forged);
    assert(!sender.encodeOutgoing(forged, wire, sizeof(wire), written));

    // Failed decode clears output so callers cannot accidentally reuse stale data.
    memset(&decoded, 0xA5, sizeof(decoded));
    assert(!receiver.decodeIncoming(altered, sizeof(altered), decoded));
    assert(decoded.bootId == 0U && decoded.sequence == 0U);

    clearPeerDirectionalKeys(keys);
    sender.clear();
    receiver.clear();
    assert(!sender.isValid() && !receiver.isValid());
    puts("PASS: PeerStatusSessionCodec HMAC, direction and boot binding tests");
    return 0;
}
