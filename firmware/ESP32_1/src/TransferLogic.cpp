#include "TransferLogic.h"
#include "WaterMath.h"
#include <math.h>

namespace HWC {

static bool finiteValue(float value) { return isfinite(value) != 0; }

TransferLogic::TransferLogic()
    : state_(TRANSFER_IDLE), endReason_(TRANSFER_REASON_NONE), startedAtMs_(0),
      startLowerLevelCm_(0.0f), targetLowerLevelCm_(0.0f), estimatedTransferredLiters_(0.0f), seenRequestIndex_(0), seenRequestCount_(0) {
    for (size_t i = 0; i < TRANSFER_RECENT_REQUEST_ID_CAPACITY; ++i) seenRequestIds_[i] = 0;
    request_.requestId = 0;
    request_.source = TRANSFER_SOURCE_NONE;
    request_.requestedLiters = 0.0f;
    request_.timeoutMs = 0;
}

bool TransferLogic::hasSeenRequestId(uint32_t requestId) const {
    if (requestId == 0) return false;
    for (size_t i = 0; i < TRANSFER_RECENT_REQUEST_ID_CAPACITY; ++i) if (seenRequestIds_[i] == requestId) return true;
    return false;
}

void TransferLogic::rememberRequestId(uint32_t requestId) {
    if (requestId == 0) return;
    seenRequestIds_[seenRequestIndex_] = requestId;
    seenRequestIndex_ = (seenRequestIndex_ + 1) % TRANSFER_RECENT_REQUEST_ID_CAPACITY;
    if (seenRequestCount_ < TRANSFER_RECENT_REQUEST_ID_CAPACITY) ++seenRequestCount_;
}

TransferEndReason TransferLogic::precheckFault(const TransferInputs &inputs, float &targetLevelCm) const {
    if (inputs.request.requestId == 0 ||
        (inputs.request.source != TRANSFER_SOURCE_WELL && inputs.request.source != TRANSFER_SOURCE_UPPER_TANK) ||
        !finiteValue(inputs.request.requestedLiters) || inputs.request.requestedLiters <= 0.0f ||
        inputs.request.timeoutMs == 0) return TRANSFER_REASON_INVALID_REQUEST;
    const bool upperSource = inputs.request.source == TRANSFER_SOURCE_UPPER_TANK;
    // Upper-tank level and pump protection are local to ESP32 #2. ESP32 #1
    // deliberately does not require a remote sample or duplicate its limits.
    if (!inputs.lowerLevelValid || !finiteValue(inputs.lowerLevelCm))
        return TRANSFER_REASON_SENSOR_INVALID;
    if (inputs.lowerLevelCm < 0.0f || inputs.lowerLevelCm > LOWER_TANK_MAX_LEVEL_CM)
        return TRANSFER_REASON_SYSTEM_NOT_READY;
    if (!upperSource) {
        if (!inputs.sourceLevelValid || !finiteValue(inputs.sourceLevelCm))
            return TRANSFER_REASON_SENSOR_INVALID;
        if (!finiteValue(inputs.sourceMinimumTransferLevelCm) ||
            !finiteValue(inputs.sourceMaximumLevelCm) ||
            inputs.sourceMaximumLevelCm <= inputs.sourceMinimumTransferLevelCm)
            return TRANSFER_REASON_SYSTEM_NOT_READY;
        if (inputs.sourceLevelCm < 0.0f || inputs.sourceLevelCm > inputs.sourceMaximumLevelCm)
            return TRANSFER_REASON_SENSOR_INVALID;
        if (inputs.sourceLevelCm <= inputs.sourceMinimumTransferLevelCm)
            return TRANSFER_REASON_SOURCE_MINIMUM;
    }
    if (inputs.criticalProtectionActive) return TRANSFER_REASON_CRITICAL_PROTECTION;
    if (!inputs.systemReady) return TRANSFER_REASON_SYSTEM_NOT_READY;
    if (inputs.request.source == TRANSFER_SOURCE_UPPER_TANK &&
        (!inputs.inflowWatchdogConfigValid || inputs.inflowStartupGraceMs == 0 ||
         inputs.inflowNoProgressTimeoutMs == 0 || !finiteValue(inputs.inflowMinimumProgressLiters) ||
         inputs.inflowMinimumProgressLiters <= 0.0f)) return TRANSFER_REASON_SYSTEM_NOT_READY;
    if (!lowerTankTargetLevelCm(inputs.lowerLevelCm, inputs.request.requestedLiters, targetLevelCm)) return TRANSFER_REASON_LOWER_CAPACITY;
    return TRANSFER_REASON_NONE;
}

TransferEndReason TransferLogic::runningFault(const TransferInputs &inputs) const {
    if (!inputs.lowerLevelValid || !finiteValue(inputs.lowerLevelCm))
        return TRANSFER_REASON_SENSOR_INVALID;
    if (request_.source == TRANSFER_SOURCE_WELL &&
        (!inputs.sourceLevelValid || !finiteValue(inputs.sourceLevelCm)))
        return TRANSFER_REASON_SENSOR_INVALID;
    // A numerically valid sensor result can still be physically impossible for the lower tank.
    // Reject it before target comparison, otherwise an over-range reading could falsely report success.
    if (inputs.lowerLevelCm < 0.0f || inputs.lowerLevelCm > LOWER_TANK_MAX_LEVEL_CM)
        return TRANSFER_REASON_SENSOR_INVALID;
    if (inputs.criticalProtectionActive) return TRANSFER_REASON_CRITICAL_PROTECTION;
    if (request_.source == TRANSFER_SOURCE_WELL) {
        if (!finiteValue(inputs.sourceMinimumTransferLevelCm) ||
            !finiteValue(inputs.sourceMaximumLevelCm) ||
            inputs.sourceMaximumLevelCm <= inputs.sourceMinimumTransferLevelCm)
            return TRANSFER_REASON_SYSTEM_NOT_READY;
        if (inputs.sourceLevelCm < 0.0f || inputs.sourceLevelCm > inputs.sourceMaximumLevelCm)
            return TRANSFER_REASON_SENSOR_INVALID;
        if (inputs.sourceLevelCm <= inputs.sourceMinimumTransferLevelCm)
            return TRANSFER_REASON_SOURCE_MINIMUM;
    }
    if (!inputs.systemReady) return TRANSFER_REASON_SYSTEM_NOT_READY;
    if (inputs.lowerLevelCm >= LOWER_TANK_MAX_LEVEL_CM && inputs.lowerLevelCm < targetLowerLevelCm_) return TRANSFER_REASON_LOWER_CAPACITY;
    return TRANSFER_REASON_NONE;
}

void TransferLogic::finish(TransferState terminalState, TransferEndReason reason) {
    state_ = terminalState;
    endReason_ = reason;
}

TransferOutputs TransferLogic::makeOutputs() const {
    TransferOutputs out;
    out.state = state_;
    out.valveCommand = VALVES_CLOSE_ALL;
    if (state_ == TRANSFER_RUNNING) {
        if (request_.source == TRANSFER_SOURCE_WELL) out.valveCommand = VALVE_OPEN_WELL_TO_LOWER;
        else if (request_.source == TRANSFER_SOURCE_UPPER_TANK) out.valveCommand = VALVE_OPEN_UPPER_TO_LOWER;
    }
    out.endReason = endReason_;
    out.requestId = request_.requestId;
    out.targetLowerLevelCm = targetLowerLevelCm_;
    out.estimatedTransferredLiters = estimatedTransferredLiters_;
    return out;
}

TransferOutputs TransferLogic::update(const TransferInputs &inputs) {
    if (state_ == TRANSFER_IDLE) {
        if (inputs.startRequested) {
            request_ = inputs.request;
            endReason_ = TRANSFER_REASON_NONE;
            estimatedTransferredLiters_ = 0.0f;
            float target = 0.0f;
            if (hasSeenRequestId(inputs.request.requestId)) {
                targetLowerLevelCm_ = 0.0f;
                finish(TRANSFER_ABORTED, TRANSFER_REASON_DUPLICATE_REQUEST);
                return makeOutputs();
            }
            rememberRequestId(inputs.request.requestId);
            const TransferEndReason fault = precheckFault(inputs, target);
            targetLowerLevelCm_ = target;
            if (fault != TRANSFER_REASON_NONE) {
                finish(TRANSFER_ABORTED, fault);
            } else {
                startLowerLevelCm_ = inputs.lowerLevelCm;
                startedAtMs_ = inputs.nowMs;
                state_ = TRANSFER_PRECHECK;
            }
        }
        return makeOutputs();
    }

    if (state_ == TRANSFER_PRECHECK) {
        if (inputs.stopRequested) {
            finish(TRANSFER_STOPPED, TRANSFER_REASON_USER_STOP);
            return makeOutputs();
        }
        float target = 0.0f;
        const TransferEndReason fault = precheckFault(inputs, target);
        if (fault != TRANSFER_REASON_NONE) {
            finish(TRANSFER_ABORTED, fault);
        } else {
            // Rebase to the freshest reading immediately before opening a valve.
            startLowerLevelCm_ = inputs.lowerLevelCm;
            targetLowerLevelCm_ = target;
            startedAtMs_ = inputs.nowMs;
            if (request_.source == TRANSFER_SOURCE_UPPER_TANK) {
                float baselineVolume = 0.0f;
                LowerTankInflowWatchdogConfig watchdogConfig = {
                    inputs.inflowStartupGraceMs,
                    inputs.inflowNoProgressTimeoutMs,
                    inputs.inflowMinimumProgressLiters
                };
                if (!lowerTankVolumeLiters(startLowerLevelCm_, baselineVolume) ||
                    !inflowWatchdog_.begin(inputs.nowMs, baselineVolume, watchdogConfig)) {
                    finish(TRANSFER_ABORTED, TRANSFER_REASON_SYSTEM_NOT_READY);
                    return makeOutputs();
                }
            }
            state_ = TRANSFER_RUNNING;
        }
        return makeOutputs();
    }

    if (state_ == TRANSFER_RUNNING) {
        // Preserve the latest valid lower-tank estimate even when STOP or a fault arrives now.
        float currentVolume = 0.0f;
        float startVolume = 0.0f;
        if (inputs.lowerLevelValid && finiteValue(inputs.lowerLevelCm) &&
            lowerTankVolumeLiters(inputs.lowerLevelCm, currentVolume) &&
            lowerTankVolumeLiters(startLowerLevelCm_, startVolume)) {
            estimatedTransferredLiters_ = currentVolume > startVolume ? currentVolume - startVolume : 0.0f;
        }
        if (inputs.stopRequested) {
            finish(TRANSFER_STOPPED, TRANSFER_REASON_USER_STOP);
            return makeOutputs();
        }
        const TransferEndReason fault = runningFault(inputs);
        if (fault != TRANSFER_REASON_NONE) {
            finish(TRANSFER_ABORTED, fault);
            return makeOutputs();
        }
        if (inputs.lowerLevelCm >= targetLowerLevelCm_) {
            finish(TRANSFER_COMPLETED, TRANSFER_REASON_TARGET_REACHED);
        } else if (request_.source == TRANSFER_SOURCE_UPPER_TANK) {
            float currentVolume = 0.0f;
            if (!lowerTankVolumeLiters(inputs.lowerLevelCm, currentVolume)) {
                finish(TRANSFER_ABORTED, TRANSFER_REASON_SENSOR_INVALID);
            } else {
                const LowerTankInflowStatus inflow = inflowWatchdog_.update(
                    inputs.nowMs, currentVolume, inputs.lowerLevelValid);
                if (inflow == LOWER_INFLOW_NO_PROGRESS) {
                    finish(TRANSFER_ABORTED, TRANSFER_REASON_NO_INFLOW);
                } else if (inflow == LOWER_INFLOW_INVALID_SAMPLE || inflow == LOWER_INFLOW_INVALID_CONFIG) {
                    finish(TRANSFER_ABORTED, TRANSFER_REASON_SENSOR_INVALID);
                } else if ((uint32_t)(inputs.nowMs - startedAtMs_) >= request_.timeoutMs) {
                    finish(TRANSFER_ABORTED, TRANSFER_REASON_TIMEOUT);
                }
            }
        } else if ((uint32_t)(inputs.nowMs - startedAtMs_) >= request_.timeoutMs) {
            finish(TRANSFER_ABORTED, TRANSFER_REASON_TIMEOUT);
        }
        return makeOutputs();
    }

    // Terminal state is held until the caller has logged the result and acknowledges it.
    return makeOutputs();
}

TransferOutputs TransferLogic::forceAbort(TransferEndReason reason) {
    if (state_ == TRANSFER_PRECHECK || state_ == TRANSFER_RUNNING) {
        finish(TRANSFER_ABORTED, reason == TRANSFER_REASON_NONE ? TRANSFER_REASON_SYSTEM_NOT_READY : reason);
    }
    return makeOutputs();
}

void TransferLogic::acknowledgeResult() {
    if (state_ == TRANSFER_COMPLETED || state_ == TRANSFER_STOPPED || state_ == TRANSFER_ABORTED) {
        state_ = TRANSFER_IDLE;
        endReason_ = TRANSFER_REASON_NONE;
        request_.requestId = 0;
        request_.source = TRANSFER_SOURCE_NONE;
        request_.requestedLiters = 0.0f;
        request_.timeoutMs = 0;
        inflowWatchdog_.reset();
        targetLowerLevelCm_ = 0.0f;
        estimatedTransferredLiters_ = 0.0f;
    }
}

bool TransferLogic::restoreRecentRequestIds(const uint32_t *requestIds, size_t count) {
    if (count > 0 && requestIds == 0) return false;

    // Preserve IDs observed since boot (for example while NVS was temporarily
    // unavailable). Persisted IDs are older, so append the in-memory window last.
    uint32_t runtimeIds[TRANSFER_RECENT_REQUEST_ID_CAPACITY] = {};
    const size_t runtimeCount = seenRequestCount_;
    const size_t runtimeFirst = runtimeCount == TRANSFER_RECENT_REQUEST_ID_CAPACITY
        ? seenRequestIndex_ : 0;
    for (size_t i = 0; i < runtimeCount; ++i) {
        runtimeIds[i] = seenRequestIds_[(runtimeFirst + i) % TRANSFER_RECENT_REQUEST_ID_CAPACITY];
    }

    for (size_t i = 0; i < TRANSFER_RECENT_REQUEST_ID_CAPACITY; ++i) seenRequestIds_[i] = 0;
    seenRequestIndex_ = 0;
    seenRequestCount_ = 0;

    const size_t first = count > TRANSFER_RECENT_REQUEST_ID_CAPACITY
        ? count - TRANSFER_RECENT_REQUEST_ID_CAPACITY : 0;
    for (size_t i = first; i < count; ++i) {
        if (requestIds[i] != 0 && !hasSeenRequestId(requestIds[i])) rememberRequestId(requestIds[i]);
    }
    for (size_t i = 0; i < runtimeCount; ++i) {
        if (runtimeIds[i] != 0 && !hasSeenRequestId(runtimeIds[i])) rememberRequestId(runtimeIds[i]);
    }
    return true;
}

TransferState TransferLogic::state() const { return state_; }

} // namespace HWC
