#ifndef HWC_NVS_DAILY_INFLOW_STORE_H
#define HWC_NVS_DAILY_INFLOW_STORE_H

#include "WaterMath.h"
#include <Preferences.h>

namespace HWC {

static const uint32_t DAILY_INFLOW_STORE_MAGIC = 0x48494331UL; // HIC1
static const uint16_t DAILY_INFLOW_STORE_VERSION = 1;

struct DailyInflowRecord {
    uint32_t magic;
    uint16_t version;
    DailyInflowSnapshot snapshot;
    uint32_t checksum;
};

uint32_t dailyInflowRecordChecksum(const DailyInflowRecord& record);
bool isDailyInflowRecordValid(const DailyInflowRecord& record);

class NvsDailyInflowStore {
public:
    NvsDailyInflowStore();
    bool begin();
    void end();
    bool hasRecord();
    bool load(DailyInflowSnapshot& out);
    bool save(const DailyInflowSnapshot& snapshot);
private:
    Preferences preferences_;
    bool opened_;
};

} // namespace HWC
#endif
