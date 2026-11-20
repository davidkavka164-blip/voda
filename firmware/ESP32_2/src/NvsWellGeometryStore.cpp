#include "NvsWellGeometryStore.h"

namespace HWC {
NvsWellGeometryStore::NvsWellGeometryStore() : opened_(false) {}

bool NvsWellGeometryStore::begin() {
    if (opened_) return true;
    opened_ = preferences_.begin("hwc_well", false);
    return opened_;
}

void NvsWellGeometryStore::end() {
    if (opened_) preferences_.end();
    opened_ = false;
}

bool NvsWellGeometryStore::load(WellGeometryConfig& out) {
    if (!opened_ || !preferences_.isKey("geometry")) return false;
    if (preferences_.getBytesLength("geometry") != sizeof(WellGeometryConfig)) return false;
    WellGeometryConfig candidate = {};
    if (preferences_.getBytes("geometry", &candidate, sizeof(candidate)) != sizeof(candidate)) return false;
    if (!isWellGeometryConfigValid(candidate) || !wellGeometryChecksumMatches(candidate)) return false;
    out = candidate;
    return true;
}

bool NvsWellGeometryStore::save(WellGeometryConfig config) {
    if (!opened_ || !isWellGeometryConfigValid(config)) return false;
    if (preferences_.isKey("geometry")) {
        WellGeometryConfig existing = {};
        if (!load(existing)) return false;
    }
    updateWellGeometryChecksum(config);
    if (preferences_.putBytes("geometry", &config, sizeof(config)) != sizeof(config)) return false;

    // Confirm the persisted geometry, not only the byte count returned by NVS.
    WellGeometryConfig verify = {};
    return load(verify) &&
           verify.magic == config.magic &&
           verify.version == config.version &&
           verify.internalDiameterCm == config.internalDiameterCm &&
           verify.usableDepthCm == config.usableDepthCm &&
           verify.checksum == config.checksum;
}
}
