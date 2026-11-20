#include "NvsUpperTankVolumeCalibrationStore.h"
namespace HWC {
namespace {
struct Record {
    uint32_t magic;
    uint16_t version;
    UpperTankVolumeCalibration calibration;
    uint32_t checksum;
};
uint32_t crcByte(uint32_t crc, uint8_t byte) {
    crc ^= byte;
    for (uint8_t i = 0; i < 8; ++i) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1U)));
        crc = (crc >> 1) ^ (0xEDB88320UL & mask);
    }
    return crc;
}
template<typename T> uint32_t crcValue(uint32_t crc, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) crc = crcByte(crc, bytes[i]);
    return crc;
}
uint32_t recordChecksum(const Record& r) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcValue(crc, r.magic);
    crc = crcValue(crc, r.version);
    crc = crcValue(crc, r.calibration.magic);
    crc = crcValue(crc, r.calibration.version);
    crc = crcValue(crc, r.calibration.pointCount);
    crc = crcValue(crc, r.calibration.absoluteAnchorKnown);
    crc = crcValue(crc, r.calibration.transferReferenceValid);
    crc = crcValue(crc, r.calibration.transferReferenceLevelCm);
    crc = crcValue(crc, r.calibration.transferReferenceVolumeLiters);
    crc = crcValue(crc, r.calibration.points);
    crc = crcValue(crc, r.calibration.checksum);
    return crc ^ 0xFFFFFFFFUL;
}
bool validRecord(const Record& r) {
    return r.magic == 0x48555652UL && r.version == 1 &&
        isUpperTankVolumeCalibrationValid(r.calibration) &&
        upperTankVolumeCalibrationChecksumMatches(r.calibration) &&
        r.checksum == recordChecksum(r);
}
bool sameCalibration(const UpperTankVolumeCalibration& a,
                     const UpperTankVolumeCalibration& b) {
    if (a.magic != b.magic || a.version != b.version ||
        a.pointCount != b.pointCount ||
        a.absoluteAnchorKnown != b.absoluteAnchorKnown ||
        a.transferReferenceValid != b.transferReferenceValid ||
        a.transferReferenceLevelCm != b.transferReferenceLevelCm ||
        a.transferReferenceVolumeLiters != b.transferReferenceVolumeLiters ||
        a.checksum != b.checksum) return false;
    for (size_t i = 0; i < a.pointCount; ++i) {
        if (a.points[i].levelCm != b.points[i].levelCm ||
            a.points[i].volumeLiters != b.points[i].volumeLiters) return false;
    }
    return true;
}
}
NvsUpperTankVolumeCalibrationStore::NvsUpperTankVolumeCalibrationStore() : opened_(false) {}
bool NvsUpperTankVolumeCalibrationStore::begin() {
    if (opened_) return true;
    opened_ = preferences_.begin("hwc_up_vol", false);
    return opened_;
}
void NvsUpperTankVolumeCalibrationStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}
bool NvsUpperTankVolumeCalibrationStore::hasRecord() {
    return opened_ && preferences_.isKey("curve");
}
bool NvsUpperTankVolumeCalibrationStore::load(UpperTankVolumeCalibration& calibration) {
    if (!hasRecord() || preferences_.getBytesLength("curve") != sizeof(Record)) return false;
    Record r = {};
    if (preferences_.getBytes("curve", &r, sizeof(r)) != sizeof(r) || !validRecord(r)) return false;
    calibration = r.calibration;
    return true;
}
bool NvsUpperTankVolumeCalibrationStore::discardCorruptRecord() {
    if (!opened_ || !hasRecord()) return false;
    const size_t storedLength = preferences_.getBytesLength("curve");
    // A non-zero, wrong-sized blob is deterministically malformed. A zero
    // length may also mean a failed metadata read, so do not delete it.
    if (storedLength == 0U) return false;
    if (storedLength == sizeof(Record)) {
        Record existing = {};
        // A failed/short read is an I/O uncertainty, not proof of corruption.
        if (preferences_.getBytes("curve", &existing, sizeof(existing)) != sizeof(existing)) return false;
        // Refuse destructive recovery if the fully read record is valid.
        if (validRecord(existing)) return false;
    }
    // Only a fully read invalid record or a non-zero malformed blob reaches here.
    if (!preferences_.remove("curve")) return false;
    // A successful API return is not enough: confirm that the key is absent.
    return !hasRecord();
}
bool NvsUpperTankVolumeCalibrationStore::save(const UpperTankVolumeCalibration& calibration) {
    if (!opened_ || !isUpperTankVolumeCalibrationValid(calibration) ||
        !upperTankVolumeCalibrationChecksumMatches(calibration)) return false;
    if (hasRecord()) {
        UpperTankVolumeCalibration old = emptyUpperTankVolumeCalibration();
        if (!load(old)) return false;
    }
    Record r = {};
    r.magic = 0x48555652UL;
    r.version = 1;
    r.calibration = calibration;
    r.checksum = recordChecksum(r);
    if (preferences_.putBytes("curve", &r, sizeof(r)) != sizeof(r)) return false;
    // Verify every persisted field, including a pending transfer reference.
    // The reference is operational state: losing it could make the next
    // post-transfer calibration point mathematically invalid.
    UpperTankVolumeCalibration verify = emptyUpperTankVolumeCalibration();
    return load(verify) && sameCalibration(verify, calibration);
}
}
