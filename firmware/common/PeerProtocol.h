#ifndef HWC_PEER_PROTOCOL_H
#define HWC_PEER_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

namespace HWC {

static const uint32_t PEER_STATUS_MAGIC = 0x48574350UL; // HWCP
static const uint16_t PEER_PROTOCOL_VERSION = 2;
static const uint8_t PEER_MESSAGE_STATUS = 1;
static const uint8_t PEER_DEVICE_UPPER_TANK = 2;
static const size_t PEER_STATUS_WIRE_SIZE = 52;
static const size_t PEER_RETIRED_BOOT_ID_CAPACITY = 4;

struct PeerStatusFrame {
    uint32_t magic;
    uint16_t protocolVersion;
    uint8_t deviceId;
    uint8_t messageType;
    uint32_t bootId;
    uint32_t sequence;
    uint32_t uptimeMs;
    uint32_t measurementAgeMs;
    float upperLevelCm;
    float upperVolumeLiters;
    float temperatureC;
    uint8_t upperLevelValid;
    uint8_t upperVolumeValid;
    uint8_t temperatureValid;
    uint8_t pumpProtectionReady;
    uint32_t upperLimitsChecksum; // 0 means upper unit has no valid saved limits
    uint32_t alarmBits;
    uint32_t checksum;
};

PeerStatusFrame makeEmptyPeerStatus(uint32_t bootId, uint32_t sequence, uint32_t uptimeMs);
uint32_t peerStatusChecksum(const PeerStatusFrame& frame);
void updatePeerStatusChecksum(PeerStatusFrame& frame);
bool isPeerStatusFrameValid(const PeerStatusFrame& frame, uint8_t expectedDeviceId);
bool encodePeerStatusFrame(const PeerStatusFrame& frame, uint8_t* out, size_t capacity, size_t& written);
bool decodePeerStatusFrame(const uint8_t* data, size_t length, uint8_t expectedDeviceId, PeerStatusFrame& out);

class PeerStatusMonitor {
public:
    PeerStatusMonitor();
    void reset();
    bool accept(const PeerStatusFrame& frame, uint8_t expectedDeviceId, uint32_t receivedAtMs);
    bool consumeBootEpochChanged(); // True once after an accepted frame starts a new peer boot epoch.
    bool hasFrame() const;
    bool isFresh(uint32_t nowMs, uint32_t maxSilenceMs) const;
    bool upperLevelUsable(uint32_t nowMs, uint32_t maxAgeMs) const;
    bool upperVolumeUsable(uint32_t nowMs, uint32_t maxAgeMs) const;
    bool temperatureUsable(uint32_t nowMs, uint32_t maxAgeMs) const;
    const PeerStatusFrame& frame() const;
    uint32_t ageSinceReceiveMs(uint32_t nowMs) const;
private:
    PeerStatusFrame frame_;
    uint32_t receivedAtMs_;
    bool hasFrame_;
    bool bootEpochChanged_;
    uint32_t retiredBootIds_[PEER_RETIRED_BOOT_ID_CAPACITY];
    size_t retiredBootIndex_;
    bool isRetiredBootId(uint32_t bootId) const;
};

} // namespace HWC
#endif
