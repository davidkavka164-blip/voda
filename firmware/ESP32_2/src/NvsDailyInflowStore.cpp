#include "NvsDailyInflowStore.h"
#include <math.h>

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

bool sameSnapshot(const DailyInflowSnapshot& a, const DailyInflowSnapshot& b) {
    if (a.baselineValid != b.baselineValid ||
        a.previousDateValid != b.previousDateValid ||
        a.previousVolumeLiters != b.previousVolumeLiters ||
        a.undatedLiters != b.undatedLiters) return false;
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        if (a.days[i].dateKey != b.days[i].dateKey ||
            a.days[i].liters != b.days[i].liters) return false;
    }
    return true;
}

bool validSnapshot(const DailyInflowSnapshot& snapshot) {
    DailyInflowTracker validator;
    return validator.restoreSnapshot(snapshot);
}
}

uint32_t dailyInflowRecordChecksum(const DailyInflowRecord& record) {
    uint32_t crc = 0xFFFFFFFFUL;
    crc = crcValue(crc, record.magic);
    crc = crcValue(crc, record.version);
    crc = crcValue(crc, record.snapshot.baselineValid);
    crc = crcValue(crc, record.snapshot.previousDateValid);
    crc = crcValue(crc, record.snapshot.previousVolumeLiters);
    for (size_t i = 0; i < INFLOW_HISTORY_DAYS; ++i) {
        crc = crcValue(crc, record.snapshot.days[i].dateKey);
        crc = crcValue(crc, record.snapshot.days[i].liters);
    }
    crc = crcValue(crc, record.snapshot.undatedLiters);
    return crc ^ 0xFFFFFFFFUL;
}

bool isDailyInflowRecordValid(const DailyInflowRecord& record) {
    return record.magic == DAILY_INFLOW_STORE_MAGIC &&
           record.version == DAILY_INFLOW_STORE_VERSION &&
           validSnapshot(record.snapshot) &&
           record.checksum == dailyInflowRecordChecksum(record);
}

NvsDailyInflowStore::NvsDailyInflowStore() : opened_(false) {}

bool NvsDailyInflowStore::begin() {
    if (opened_) return true;
    opened_ = preferences_.begin("hwc_inflow", false);
    return opened_;
}

void NvsDailyInflowStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsDailyInflowStore::hasRecord() {
    return opened_ && preferences_.isKey("history");
}

bool NvsDailyInflowStore::load(DailyInflowSnapshot& out) {
    if (!opened_ || !preferences_.isKey("history")) return false;
    if (preferences_.getBytesLength("history") != sizeof(DailyInflowRecord)) return false;
    DailyInflowRecord candidate = {};
    if (preferences_.getBytes("history", &candidate, sizeof(candidate)) != sizeof(candidate)) return false;
    if (!isDailyInflowRecordValid(candidate)) return false;
    out = candidate.snapshot;
    return true;
}

bool NvsDailyInflowStore::save(const DailyInflowSnapshot& snapshot) {
    if (!opened_ || !validSnapshot(snapshot)) return false;
    // Never overwrite an existing record that fails validation/checksum.
    if (preferences_.isKey("history")) {
        DailyInflowSnapshot existing = {};
        if (!load(existing)) return false;
    }
    DailyInflowRecord record = {};
    record.magic = DAILY_INFLOW_STORE_MAGIC;
    record.version = DAILY_INFLOW_STORE_VERSION;
    record.snapshot = snapshot;
    record.checksum = dailyInflowRecordChecksum(record);
    if (preferences_.putBytes("history", &record, sizeof(record)) != sizeof(record)) return false;
    DailyInflowSnapshot verify = {};
    return load(verify) && sameSnapshot(verify, snapshot);
}

} // namespace HWC
