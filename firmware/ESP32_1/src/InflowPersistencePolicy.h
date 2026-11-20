#ifndef HWC_INFLOW_PERSISTENCE_POLICY_H
#define HWC_INFLOW_PERSISTENCE_POLICY_H

#include <stdint.h>

namespace HWC {

static const uint32_t INFLOW_PERSISTENCE_INTERVAL_MS = 300000UL; // 5 minutes

struct InflowPersistencePolicy {
    bool initialized;
    bool lastDateValid;
    uint32_t lastDateKey;
    bool dirty;
    bool saveClockStarted;
    uint32_t lastSaveMs;
};

InflowPersistencePolicy defaultInflowPersistencePolicy();
void observeInflowPersistence(InflowPersistencePolicy& policy, bool positiveChange,
                              uint32_t dateKey, bool dateValid);
bool inflowPersistenceDue(const InflowPersistencePolicy& policy, bool storageWritable,
                          uint32_t nowMs);
void markInflowPersisted(InflowPersistencePolicy& policy, uint32_t nowMs);

} // namespace HWC

#endif
