#include "NvsSensorConfigStore.h"

namespace HWC {
namespace {
bool sameSensorConfig(const SensorConfig& a, const SensorConfig& b) {
    return a.magic == b.magic &&
           a.version == b.version &&
           a.shuntOhms == b.shuntOhms &&
           a.currentMinMilliAmps == b.currentMinMilliAmps &&
           a.currentMaxMilliAmps == b.currentMaxMilliAmps &&
           a.currentToleranceMilliAmps == b.currentToleranceMilliAmps &&
           a.sensorRangeCm == b.sensorRangeCm &&
           a.calibrationScale == b.calibrationScale &&
           a.calibrationOffsetCm == b.calibrationOffsetCm &&
           a.checksum == b.checksum;
}
} // namespace

NvsSensorConfigStore::NvsSensorConfigStore() : opened_(false) {}

bool NvsSensorConfigStore::begin() {
    if (opened_) return true;
    opened_ = preferences_.begin("hwc_cfg", false);
    return opened_;
}

void NvsSensorConfigStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsSensorConfigStore::hasRecord(const char* key) {
    return opened_ && key != nullptr && preferences_.isKey(key);
}

bool NvsSensorConfigStore::load(const char* key, SensorConfig& out) {
    if (!opened_ || key == nullptr || !preferences_.isKey(key)) return false;
    if (preferences_.getBytesLength(key) != sizeof(SensorConfig)) return false;
    SensorConfig candidate = {};
    if (preferences_.getBytes(key, &candidate, sizeof(candidate)) != sizeof(candidate)) return false;
    if (!isSensorConfigValid(candidate) || !sensorConfigChecksumMatches(candidate)) return false;
    out = candidate;
    return true;
}

bool NvsSensorConfigStore::save(const char* key, SensorConfig config) {
    if (!opened_ || key == nullptr || !isSensorConfigValid(config)) return false;
    // Protect existing custom calibration from being silently replaced if its record is corrupt.
    if (preferences_.isKey(key)) {
        SensorConfig existing = defaultSensorConfig();
        if (!load(key, existing)) return false;
    }
    updateSensorConfigChecksum(config);
    if (preferences_.putBytes(key, &config, sizeof(config)) != sizeof(config)) return false;

    // A successful Preferences write count is not enough for this safety-relevant
    // calibration: read the complete record back and verify its checksum/value.
    SensorConfig verify = {};
    return load(key, verify) &&
           sensorConfigChecksumMatches(verify) &&
           sameSensorConfig(verify, config);
}

bool NvsSensorConfigStore::recover(const char* key, SensorConfig config) {
    if (!opened_ || key == nullptr || !isSensorConfigValid(config)) return false;

    // Recovery is deliberately narrower than normal calibration editing:
    // missing and corrupt records may be replaced, but a valid record may not.
    if (preferences_.isKey(key)) {
        const size_t storedLength = preferences_.getBytesLength(key);
        if (storedLength == sizeof(SensorConfig)) {
            SensorConfig existing = {};
            // If the record cannot be read completely, its invalidity is not
            // proven. Refuse recovery rather than risk overwriting valid data.
            if (preferences_.getBytes(key, &existing, sizeof(existing)) != sizeof(existing)) return false;
            if (isSensorConfigValid(existing) && sensorConfigChecksumMatches(existing)) return false;
        }
        // A present record with a different byte length is provably malformed.
    }

    updateSensorConfigChecksum(config);
    if (!isSensorConfigValid(config) || !sensorConfigChecksumMatches(config)) return false;
    if (preferences_.putBytes(key, &config, sizeof(config)) != sizeof(config)) return false;

    SensorConfig verify = {};
    return load(key, verify) &&
           sensorConfigChecksumMatches(verify) &&
           sameSensorConfig(verify, config);
}

} // namespace HWC
