#include "LowerTankInflowWatchdog.h"
#include <math.h>

namespace HWC {

LowerTankInflowWatchdog::LowerTankInflowWatchdog() { reset(); }

void LowerTankInflowWatchdog::reset() {
    status_ = LOWER_INFLOW_NOT_STARTED;
    config_.startupGraceMs = 0;
    config_.noProgressTimeoutMs = 0;
    config_.minimumProgressLiters = 0.0f;
    startedAtMs_ = 0;
    lastProgressAtMs_ = 0;
    baselineVolumeLiters_ = 0.0f;
    lastProgressVolumeLiters_ = 0.0f;
    observedIncreaseLiters_ = 0.0f;
}

bool LowerTankInflowWatchdog::begin(uint32_t nowMs, float baselineVolumeLiters,
                                    const LowerTankInflowWatchdogConfig &config) {
    reset();
    if (!isfinite(baselineVolumeLiters) || baselineVolumeLiters < 0.0f ||
        config.startupGraceMs == 0 || config.noProgressTimeoutMs == 0 ||
        !isfinite(config.minimumProgressLiters) || config.minimumProgressLiters <= 0.0f) {
        status_ = LOWER_INFLOW_INVALID_CONFIG;
        return false;
    }
    config_ = config;
    startedAtMs_ = nowMs;
    lastProgressAtMs_ = nowMs;
    baselineVolumeLiters_ = baselineVolumeLiters;
    lastProgressVolumeLiters_ = baselineVolumeLiters;
    status_ = LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS;
    return true;
}

LowerTankInflowStatus LowerTankInflowWatchdog::update(uint32_t nowMs, float lowerVolumeLiters,
                                                       bool sampleValid) {
    if (status_ != LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS && status_ != LOWER_INFLOW_CONFIRMED)
        return status_;
    if (!sampleValid || !isfinite(lowerVolumeLiters) || lowerVolumeLiters < 0.0f) {
        status_ = LOWER_INFLOW_INVALID_SAMPLE;
        return status_;
    }

    observedIncreaseLiters_ = lowerVolumeLiters > baselineVolumeLiters_
        ? lowerVolumeLiters - baselineVolumeLiters_ : 0.0f;

    const uint32_t startupElapsedMs = static_cast<uint32_t>(nowMs - startedAtMs_);
    // If the first observation arrives after the startup window, do not accept
    // late progress as proof that flow began on time. Sampling is periodic, so
    // a sample exactly at the configured deadline is still eligible.
    if (status_ == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS &&
        startupElapsedMs > config_.startupGraceMs) {
        status_ = LOWER_INFLOW_NO_PROGRESS;
        return status_;
    }

    // Only count positive progress beyond the configured noise/deadband.
    // Small fluctuations and falling levels never reset the no-progress timer.
    if (lowerVolumeLiters - lastProgressVolumeLiters_ >= config_.minimumProgressLiters) {
        lastProgressVolumeLiters_ = lowerVolumeLiters;
        lastProgressAtMs_ = nowMs;
        status_ = LOWER_INFLOW_CONFIRMED;
    }

    if (status_ == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS) {
        if (startupElapsedMs >= config_.startupGraceMs)
            status_ = LOWER_INFLOW_NO_PROGRESS;
    } else if (static_cast<uint32_t>(nowMs - lastProgressAtMs_) >= config_.noProgressTimeoutMs) {
        status_ = LOWER_INFLOW_NO_PROGRESS;
    }
    return status_;
}

LowerTankInflowStatus LowerTankInflowWatchdog::status() const { return status_; }
float LowerTankInflowWatchdog::observedIncreaseLiters() const { return observedIncreaseLiters_; }

} // namespace HWC
