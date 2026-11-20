#include "InflowPersistencePolicy.h"

namespace HWC {

InflowPersistencePolicy defaultInflowPersistencePolicy() {
    InflowPersistencePolicy p = {};
    return p;
}

void observeInflowPersistence(InflowPersistencePolicy& p, bool positiveChange,
                              uint32_t dateKey, bool dateValid) {
    const bool validDate = dateValid && dateKey != 0;
    const bool dateChanged = validDate &&
        (!p.initialized || !p.lastDateValid || p.lastDateKey != dateKey);
    if (positiveChange || dateChanged) p.dirty = true;
    p.initialized = true;
    p.lastDateValid = validDate;
    p.lastDateKey = validDate ? dateKey : 0;
}

bool inflowPersistenceDue(const InflowPersistencePolicy& p, bool storageWritable,
                          uint32_t nowMs) {
    if (!storageWritable || !p.dirty) return false;
    if (!p.saveClockStarted) return true;
    return static_cast<uint32_t>(nowMs - p.lastSaveMs) >= INFLOW_PERSISTENCE_INTERVAL_MS;
}

void markInflowPersisted(InflowPersistencePolicy& p, uint32_t nowMs) {
    p.dirty = false;
    p.saveClockStarted = true;
    p.lastSaveMs = nowMs;
}

} // namespace HWC
