#include "PeerProtocol.h"
#include <math.h>
#include <string.h>

namespace HWC {
namespace {
uint32_t crcByte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; ++bit) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}

template <typename T>
uint32_t crcValue(uint32_t crc, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) crc = crcByte(crc, bytes[i]);
    return crc;
}

bool flagValid(uint8_t value) { return value <= 1U; }

// RFC-1982-style serial arithmetic. A difference of exactly half the
// uint32_t range is ambiguous and is deliberately rejected. This avoids
// implementation-defined uint32_t-to-int32_t conversion at rollover.
bool sequenceIsNewer(uint32_t candidate, uint32_t current) {
    const uint32_t delta = static_cast<uint32_t>(candidate - current);
    return delta != 0U && delta < 0x80000000UL;
}

bool dataValuesValid(const PeerStatusFrame& f) {
    if (!isfinite(f.upperLevelCm) || !isfinite(f.upperVolumeLiters) || !isfinite(f.temperatureC)) return false;
    if (f.upperLevelValid && (f.upperLevelCm < 0.0f || f.upperLevelCm > 1000.0f)) return false;
    if (f.upperVolumeValid && (f.upperVolumeLiters < 0.0f || f.upperVolumeLiters > 100000.0f)) return false;
    if (f.temperatureValid && (f.temperatureC < -20.0f || f.temperatureC > 100.0f)) return false;
    return true;
}

bool measurementUsable(const PeerStatusFrame& f, uint32_t nowMs, uint32_t receivedAtMs,
                       bool hasFrame, uint8_t valueValid, uint32_t maxAgeMs) {
    if (!hasFrame || valueValid != 1U) return false;
    const uint32_t elapsed = static_cast<uint32_t>(nowMs - receivedAtMs);
    if (elapsed > maxAgeMs) return false;
    if (f.measurementAgeMs > maxAgeMs - elapsed) return false;
    return true;
}
}

PeerStatusFrame makeEmptyPeerStatus(uint32_t bootId, uint32_t sequence, uint32_t uptimeMs) {
    PeerStatusFrame f = {};
    f.magic = PEER_STATUS_MAGIC;
    f.protocolVersion = PEER_PROTOCOL_VERSION;
    f.deviceId = PEER_DEVICE_UPPER_TANK;
    f.messageType = PEER_MESSAGE_STATUS;
    f.bootId = bootId;
    f.sequence = sequence;
    f.uptimeMs = uptimeMs;
    updatePeerStatusChecksum(f);
    return f;
}

uint32_t peerStatusChecksum(const PeerStatusFrame& f) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcValue(crc, f.magic);
    crc = crcValue(crc, f.protocolVersion);
    crc = crcValue(crc, f.deviceId);
    crc = crcValue(crc, f.messageType);
    crc = crcValue(crc, f.bootId);
    crc = crcValue(crc, f.sequence);
    crc = crcValue(crc, f.uptimeMs);
    crc = crcValue(crc, f.measurementAgeMs);
    crc = crcValue(crc, f.upperLevelCm);
    crc = crcValue(crc, f.upperVolumeLiters);
    crc = crcValue(crc, f.temperatureC);
    crc = crcValue(crc, f.upperLevelValid);
    crc = crcValue(crc, f.upperVolumeValid);
    crc = crcValue(crc, f.temperatureValid);
    crc = crcValue(crc, f.pumpProtectionReady);
    crc = crcValue(crc, f.upperLimitsChecksum);
    crc = crcValue(crc, f.alarmBits);
    return crc ^ 0xFFFFFFFFUL;
}

void updatePeerStatusChecksum(PeerStatusFrame& f) { f.checksum = peerStatusChecksum(f); }

namespace {
void writeU16(uint8_t*& out, uint16_t value) {
    *out++ = static_cast<uint8_t>(value & 0xFFU);
    *out++ = static_cast<uint8_t>((value >> 8) & 0xFFU);
}
void writeU32(uint8_t*& out, uint32_t value) {
    *out++ = static_cast<uint8_t>(value & 0xFFU);
    *out++ = static_cast<uint8_t>((value >> 8) & 0xFFU);
    *out++ = static_cast<uint8_t>((value >> 16) & 0xFFU);
    *out++ = static_cast<uint8_t>((value >> 24) & 0xFFU);
}
uint16_t readU16(const uint8_t*& in) {
    const uint16_t value = static_cast<uint16_t>(in[0]) |
                           static_cast<uint16_t>(static_cast<uint16_t>(in[1]) << 8);
    in += 2;
    return value;
}
uint32_t readU32(const uint8_t*& in) {
    const uint32_t value = static_cast<uint32_t>(in[0]) |
                           (static_cast<uint32_t>(in[1]) << 8) |
                           (static_cast<uint32_t>(in[2]) << 16) |
                           (static_cast<uint32_t>(in[3]) << 24);
    in += 4;
    return value;
}
void writeFloat(uint8_t*& out, float value) {
    uint32_t bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    writeU32(out, bits);
}
float readFloat(const uint8_t*& in) {
    const uint32_t bits = readU32(in);
    float value = 0.0f;
    memcpy(&value, &bits, sizeof(value));
    return value;
}
}

bool encodePeerStatusFrame(const PeerStatusFrame& f, uint8_t* out, size_t capacity, size_t& written) {
    written = 0;
    if (out == nullptr || capacity < PEER_STATUS_WIRE_SIZE ||
        !isPeerStatusFrameValid(f, f.deviceId)) return false;
    uint8_t* p = out;
    writeU32(p, f.magic);
    writeU16(p, f.protocolVersion);
    *p++ = f.deviceId;
    *p++ = f.messageType;
    writeU32(p, f.bootId);
    writeU32(p, f.sequence);
    writeU32(p, f.uptimeMs);
    writeU32(p, f.measurementAgeMs);
    writeFloat(p, f.upperLevelCm);
    writeFloat(p, f.upperVolumeLiters);
    writeFloat(p, f.temperatureC);
    *p++ = f.upperLevelValid;
    *p++ = f.upperVolumeValid;
    *p++ = f.temperatureValid;
    *p++ = f.pumpProtectionReady;
    writeU32(p, f.upperLimitsChecksum);
    writeU32(p, f.alarmBits);
    writeU32(p, f.checksum);
    written = static_cast<size_t>(p - out);
    return written == PEER_STATUS_WIRE_SIZE;
}

bool decodePeerStatusFrame(const uint8_t* data, size_t length, uint8_t expectedDeviceId, PeerStatusFrame& out) {
    if (data == nullptr || length != PEER_STATUS_WIRE_SIZE) return false;
    const uint8_t* p = data;
    PeerStatusFrame candidate = {};
    candidate.magic = readU32(p);
    candidate.protocolVersion = readU16(p);
    candidate.deviceId = *p++;
    candidate.messageType = *p++;
    candidate.bootId = readU32(p);
    candidate.sequence = readU32(p);
    candidate.uptimeMs = readU32(p);
    candidate.measurementAgeMs = readU32(p);
    candidate.upperLevelCm = readFloat(p);
    candidate.upperVolumeLiters = readFloat(p);
    candidate.temperatureC = readFloat(p);
    candidate.upperLevelValid = *p++;
    candidate.upperVolumeValid = *p++;
    candidate.temperatureValid = *p++;
    candidate.pumpProtectionReady = *p++;
    candidate.upperLimitsChecksum = readU32(p);
    candidate.alarmBits = readU32(p);
    candidate.checksum = readU32(p);
    if (!isPeerStatusFrameValid(candidate, expectedDeviceId)) return false;
    out = candidate;
    return true;
}

bool isPeerStatusFrameValid(const PeerStatusFrame& f, uint8_t expectedDeviceId) {
    return f.magic == PEER_STATUS_MAGIC &&
           f.protocolVersion == PEER_PROTOCOL_VERSION &&
           f.deviceId == expectedDeviceId &&
           f.messageType == PEER_MESSAGE_STATUS &&
           f.bootId != 0U &&
           flagValid(f.upperLevelValid) && flagValid(f.upperVolumeValid) &&
           flagValid(f.temperatureValid) && flagValid(f.pumpProtectionReady) &&
           dataValuesValid(f) && f.checksum == peerStatusChecksum(f);
}

PeerStatusMonitor::PeerStatusMonitor() { reset(); }

void PeerStatusMonitor::reset() {
    frame_ = PeerStatusFrame();
    receivedAtMs_ = 0;
    hasFrame_ = false;
    bootEpochChanged_ = false;
    retiredBootIndex_ = 0;
    for (size_t i = 0; i < PEER_RETIRED_BOOT_ID_CAPACITY; ++i) retiredBootIds_[i] = 0U;
}

bool PeerStatusMonitor::isRetiredBootId(uint32_t bootId) const {
    if (bootId == 0U) return true;
    for (size_t i = 0; i < PEER_RETIRED_BOOT_ID_CAPACITY; ++i) {
        if (retiredBootIds_[i] == bootId) return true;
    }
    return false;
}

bool PeerStatusMonitor::accept(const PeerStatusFrame& frame, uint8_t expectedDeviceId, uint32_t receivedAtMs) {
    if (!isPeerStatusFrameValid(frame, expectedDeviceId)) return false;
    bool newBootEpoch = false;
    if (hasFrame_) {
        if (frame.bootId == frame_.bootId) {
            if (!sequenceIsNewer(frame.sequence, frame_.sequence)) {
                return false; // duplicate, old, out-of-order or ambiguous half-range packet
            }
        } else {
            // UDP can deliver a delayed datagram from the previous boot after
            // the first frame of a new boot has already been accepted. Retain
            // a small bounded history so that such a packet cannot roll the
            // monitor back to a recently retired boot epoch.
            if (isRetiredBootId(frame.bootId)) return false;
            newBootEpoch = true;
            retiredBootIds_[retiredBootIndex_] = frame_.bootId;
            retiredBootIndex_ = (retiredBootIndex_ + 1U) % PEER_RETIRED_BOOT_ID_CAPACITY;
        }
    }
    frame_ = frame;
    receivedAtMs_ = receivedAtMs;
    hasFrame_ = true;
    if (newBootEpoch) bootEpochChanged_ = true;
    return true;
}

bool PeerStatusMonitor::consumeBootEpochChanged() {
    const bool changed = bootEpochChanged_;
    bootEpochChanged_ = false;
    return changed;
}

bool PeerStatusMonitor::hasFrame() const { return hasFrame_; }

bool PeerStatusMonitor::isFresh(uint32_t nowMs, uint32_t maxSilenceMs) const {
    return hasFrame_ && static_cast<uint32_t>(nowMs - receivedAtMs_) <= maxSilenceMs;
}

bool PeerStatusMonitor::upperLevelUsable(uint32_t nowMs, uint32_t maxAgeMs) const {
    return measurementUsable(frame_, nowMs, receivedAtMs_, hasFrame_, frame_.upperLevelValid, maxAgeMs);
}

bool PeerStatusMonitor::upperVolumeUsable(uint32_t nowMs, uint32_t maxAgeMs) const {
    return measurementUsable(frame_, nowMs, receivedAtMs_, hasFrame_, frame_.upperVolumeValid, maxAgeMs);
}

bool PeerStatusMonitor::temperatureUsable(uint32_t nowMs, uint32_t maxAgeMs) const {
    return measurementUsable(frame_, nowMs, receivedAtMs_, hasFrame_, frame_.temperatureValid, maxAgeMs);
}

const PeerStatusFrame& PeerStatusMonitor::frame() const { return frame_; }

uint32_t PeerStatusMonitor::ageSinceReceiveMs(uint32_t nowMs) const {
    return hasFrame_ ? static_cast<uint32_t>(nowMs - receivedAtMs_) : UINT32_MAX;
}

} // namespace HWC
