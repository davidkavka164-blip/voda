#include "PeerProtocol.h"
#include <assert.h>
#include <stdio.h>

int main() {
    using namespace HWC;
    PeerStatusFrame f = makeEmptyPeerStatus(0x12345678U, 1U, 1000U);
    f.upperLevelCm = 80.0f;
    f.upperLevelValid = 1;
    f.upperVolumeLiters = 4300.0f;
    f.upperVolumeValid = 1;
    f.temperatureC = 16.5f;
    f.temperatureValid = 1;
    f.pumpProtectionReady = 1;
    f.upperLimitsChecksum = 0xA1B2C3D4UL;
    f.measurementAgeMs = 250U;
    updatePeerStatusChecksum(f);
    assert(isPeerStatusFrameValid(f, PEER_DEVICE_UPPER_TANK));
    assert(!isPeerStatusFrameValid(f, 1U));

    uint8_t wire[PEER_STATUS_WIRE_SIZE] = {};
    size_t wireLength = 0;
    assert(encodePeerStatusFrame(f, wire, sizeof(wire), wireLength));
    assert(wireLength == PEER_STATUS_WIRE_SIZE);
    PeerStatusFrame decoded = {};
    assert(decodePeerStatusFrame(wire, wireLength, PEER_DEVICE_UPPER_TANK, decoded));
    assert(decoded.sequence == f.sequence && decoded.bootId == f.bootId);
    assert(decoded.upperLevelCm == f.upperLevelCm && decoded.upperVolumeLiters == f.upperVolumeLiters);
    assert(decoded.upperLimitsChecksum == f.upperLimitsChecksum);
    assert(!decodePeerStatusFrame(wire, wireLength - 1, PEER_DEVICE_UPPER_TANK, decoded));
    uint8_t oldVersionWire[48] = {};
    assert(!decodePeerStatusFrame(oldVersionWire, sizeof(oldVersionWire), PEER_DEVICE_UPPER_TANK, decoded));
    wire[12] ^= 0x20U;
    assert(!decodePeerStatusFrame(wire, wireLength, PEER_DEVICE_UPPER_TANK, decoded));
    wire[12] ^= 0x20U;

    PeerStatusMonitor monitor;
    assert(!monitor.hasFrame());
    assert(monitor.accept(f, PEER_DEVICE_UPPER_TANK, 10000U));
    assert(!monitor.consumeBootEpochChanged()); // Initial peer discovery is not a reboot.
    assert(monitor.isFresh(12000U, 3000U));
    assert(monitor.upperLevelUsable(12000U, 3000U));
    assert(monitor.upperVolumeUsable(12000U, 3000U));
    assert(monitor.temperatureUsable(12000U, 3000U));
    assert(!monitor.isFresh(14001U, 3000U));
    assert(!monitor.upperLevelUsable(13000U, 3000U)); // measurement age + receive age > limit

    assert(!monitor.accept(f, PEER_DEVICE_UPPER_TANK, 13000U)); // duplicate sequence
    PeerStatusFrame older = f;
    older.sequence = 0;
    updatePeerStatusChecksum(older);
    assert(!monitor.accept(older, PEER_DEVICE_UPPER_TANK, 13000U));

    PeerStatusFrame next = f;
    next.sequence = 2;
    next.upperLevelValid = 0;
    next.upperVolumeValid = 0;
    updatePeerStatusChecksum(next);
    assert(monitor.accept(next, PEER_DEVICE_UPPER_TANK, 13001U));
    assert(!monitor.upperLevelUsable(13001U, 3000U));

    PeerStatusFrame rebooted = makeEmptyPeerStatus(0x87654321U, 1U, 10U);
    rebooted.upperLevelCm = 25.0f;
    rebooted.upperLevelValid = 1;
    updatePeerStatusChecksum(rebooted);
    assert(monitor.accept(rebooted, PEER_DEVICE_UPPER_TANK, 13002U)); // new boot ID resets sequence space
    assert(monitor.consumeBootEpochChanged()); // A real, accepted peer reboot is reported once.
    assert(!monitor.consumeBootEpochChanged()); // The notification is edge-triggered.

    // A delayed datagram from the just-retired boot must not roll the monitor
    // back to the previous epoch after a reboot frame has been accepted.
    PeerStatusFrame delayedOldBoot = f;
    delayedOldBoot.sequence = 3U;
    updatePeerStatusChecksum(delayedOldBoot);
    assert(!monitor.accept(delayedOldBoot, PEER_DEVICE_UPPER_TANK, 13003U));
    assert(!monitor.consumeBootEpochChanged()); // Rejected delayed frames do not signal another reboot.

    // Sequence rollover is valid: FFFFFFFF -> 0 is a forward step of one.
    PeerStatusMonitor rolloverMonitor;
    PeerStatusFrame rollover = makeEmptyPeerStatus(0x10203040U, 0xFFFFFFFEUL, 1U);
    assert(rolloverMonitor.accept(rollover, PEER_DEVICE_UPPER_TANK, 20000U));
    rollover.sequence = 0xFFFFFFFFUL;
    updatePeerStatusChecksum(rollover);
    assert(rolloverMonitor.accept(rollover, PEER_DEVICE_UPPER_TANK, 20001U));
    rollover.sequence = 0U;
    updatePeerStatusChecksum(rollover);
    assert(rolloverMonitor.accept(rollover, PEER_DEVICE_UPPER_TANK, 20002U));
    PeerStatusFrame preRolloverDelayed = rollover;
    preRolloverDelayed.sequence = 0xFFFFFFFFUL;
    updatePeerStatusChecksum(preRolloverDelayed);
    assert(!rolloverMonitor.accept(preRolloverDelayed, PEER_DEVICE_UPPER_TANK, 20003U));

    // Exactly half the serial-number space is ambiguous, not newer.
    PeerStatusFrame ambiguous = rollover;
    ambiguous.sequence = 0x80000000UL;
    updatePeerStatusChecksum(ambiguous);
    assert(!rolloverMonitor.accept(ambiguous, PEER_DEVICE_UPPER_TANK, 20004U));

    // Retired boot history is a bounded four-entry ring. The four most
    // recently retired epochs are rejected; the oldest is evicted on the
    // fifth transition, which documents the deliberate bounded-history limit.
    PeerStatusMonitor epochMonitor;
    const uint32_t bootIds[] = {101U, 202U, 303U, 404U, 505U, 606U};
    PeerStatusFrame epoch = makeEmptyPeerStatus(bootIds[0], 1U, 1U);
    assert(epochMonitor.accept(epoch, PEER_DEVICE_UPPER_TANK, 30000U));
    for (size_t i = 1; i < sizeof(bootIds) / sizeof(bootIds[0]); ++i) {
        epoch = makeEmptyPeerStatus(bootIds[i], 1U, 1U);
        assert(epochMonitor.accept(epoch, PEER_DEVICE_UPPER_TANK, 30000U + static_cast<uint32_t>(i)));
    }
    for (size_t i = 1; i < 5; ++i) {
        PeerStatusFrame retired = makeEmptyPeerStatus(bootIds[i], 2U, 2U);
        assert(!epochMonitor.accept(retired, PEER_DEVICE_UPPER_TANK, 30100U + static_cast<uint32_t>(i)));
    }
    // Epoch 101 has aged out of the four-entry history and can no longer be
    // distinguished from a genuinely new boot ID by this status-only layer.
    PeerStatusFrame evictedEpoch = makeEmptyPeerStatus(bootIds[0], 2U, 2U);
    assert(epochMonitor.accept(evictedEpoch, PEER_DEVICE_UPPER_TANK, 30200U));

    PeerStatusFrame corrupt = rebooted;
    corrupt.upperLevelCm = 2000.0f;
    assert(!isPeerStatusFrameValid(corrupt, PEER_DEVICE_UPPER_TANK)); // CRC and range both fail
    corrupt = rebooted;
    corrupt.upperLevelValid = 2;
    updatePeerStatusChecksum(corrupt);
    assert(!isPeerStatusFrameValid(corrupt, PEER_DEVICE_UPPER_TANK)); // invalid flag value

    puts("PASS: PeerProtocol native tests");
    return 0;
}
