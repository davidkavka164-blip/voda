#ifndef HWC_LOWER_TANK_INFLOW_WATCHDOG_H
#define HWC_LOWER_TANK_INFLOW_WATCHDOG_H

#include <stdint.h>

namespace HWC {

enum LowerTankInflowStatus {
    LOWER_INFLOW_NOT_STARTED,
    LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS,
    LOWER_INFLOW_CONFIRMED,
    LOWER_INFLOW_NO_PROGRESS,
    LOWER_INFLOW_INVALID_SAMPLE,
    LOWER_INFLOW_INVALID_CONFIG
};

struct LowerTankInflowWatchdogConfig {
    uint32_t startupGraceMs;
    uint32_t noProgressTimeoutMs;
    float minimumProgressLiters;
};

// Monitors a transfer indirectly through the receiving (lower) tank volume.
// It does not decide valve commands; the caller must close all valves whenever
// the returned status is terminal. Configuration values must be calibrated
// against real sensor noise and actual flow before production integration.
class LowerTankInflowWatchdog {
public:
    LowerTankInflowWatchdog();
    bool begin(uint32_t nowMs, float baselineVolumeLiters,
               const LowerTankInflowWatchdogConfig &config);
    LowerTankInflowStatus update(uint32_t nowMs, float lowerVolumeLiters, bool sampleValid);
    void reset();
    LowerTankInflowStatus status() const;
    float observedIncreaseLiters() const;

private:
    LowerTankInflowStatus status_;
    LowerTankInflowWatchdogConfig config_;
    uint32_t startedAtMs_;
    uint32_t lastProgressAtMs_;
    float baselineVolumeLiters_;
    float lastProgressVolumeLiters_;
    float observedIncreaseLiters_;
};

} // namespace HWC

#endif
