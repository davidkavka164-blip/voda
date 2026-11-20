#ifndef HWC_TRANSFER_LOGIC_H
#define HWC_TRANSFER_LOGIC_H

#include <stdint.h>
#include <stddef.h>
#include "LowerTankInflowWatchdog.h"

namespace HWC {

enum TransferSource {
    TRANSFER_SOURCE_NONE,
    TRANSFER_SOURCE_WELL,
    TRANSFER_SOURCE_UPPER_TANK
};

enum TransferState {
    TRANSFER_IDLE,
    TRANSFER_PRECHECK,
    TRANSFER_RUNNING,
    TRANSFER_COMPLETED,
    TRANSFER_STOPPED,
    TRANSFER_ABORTED
};

enum TransferValveCommand {
    VALVES_CLOSE_ALL,
    VALVE_OPEN_WELL_TO_LOWER,
    VALVE_OPEN_UPPER_TO_LOWER
};

static const size_t TRANSFER_RECENT_REQUEST_ID_CAPACITY = 12;

enum TransferEndReason {
    TRANSFER_REASON_NONE,
    TRANSFER_REASON_TARGET_REACHED,
    TRANSFER_REASON_USER_STOP,
    TRANSFER_REASON_INVALID_REQUEST,
    TRANSFER_REASON_DUPLICATE_REQUEST,
    TRANSFER_REASON_SENSOR_INVALID,
    TRANSFER_REASON_SOURCE_MINIMUM,
    TRANSFER_REASON_CRITICAL_PROTECTION,
    TRANSFER_REASON_LOWER_CAPACITY,
    TRANSFER_REASON_COMMUNICATION,
    TRANSFER_REASON_SYSTEM_NOT_READY,
    TRANSFER_REASON_TIMEOUT,
    TRANSFER_REASON_VALVE_NOT_PERMITTED,
    TRANSFER_REASON_NO_INFLOW
};

struct TransferRequest {
    uint32_t requestId;
    TransferSource source;
    float requestedLiters;
    uint32_t timeoutMs;
};

struct TransferInputs {
    uint32_t nowMs;
    bool startRequested;
    bool stopRequested;
    TransferRequest request;
    bool lowerLevelValid;
    float lowerLevelCm;
    bool sourceLevelValid;
    float sourceLevelCm;
    float sourceMinimumTransferLevelCm;
    float sourceMaximumLevelCm; // Physical/configured upper bound for the selected source.
    bool criticalProtectionActive;
    bool systemReady;
    bool inflowWatchdogConfigValid;
    uint32_t inflowStartupGraceMs;
    uint32_t inflowNoProgressTimeoutMs;
    float inflowMinimumProgressLiters;
};

struct TransferOutputs {
    TransferState state;
    TransferValveCommand valveCommand;
    TransferEndReason endReason;
    uint32_t requestId;
    float targetLowerLevelCm;
    float estimatedTransferredLiters;
};

class TransferLogic {
public:
    TransferLogic();
    TransferOutputs update(const TransferInputs &inputs);
    void acknowledgeResult();
    // Force a terminal fail-safe state; the returned valve command is always CLOSE_ALL.
    TransferOutputs forceAbort(TransferEndReason reason);
    // Restore the newest request IDs from persistent transfer history after reboot.
    // Older IDs are discarded when count exceeds the fixed replay window.
    bool restoreRecentRequestIds(const uint32_t *requestIds, size_t count);
    TransferState state() const;

private:
    TransferState state_;
    TransferRequest request_;
    TransferEndReason endReason_;
    uint32_t startedAtMs_;
    float startLowerLevelCm_;
    float targetLowerLevelCm_;
    float estimatedTransferredLiters_;
    LowerTankInflowWatchdog inflowWatchdog_;
    uint32_t seenRequestIds_[TRANSFER_RECENT_REQUEST_ID_CAPACITY];
    size_t seenRequestIndex_;
    size_t seenRequestCount_;
    bool hasSeenRequestId(uint32_t requestId) const;
    void rememberRequestId(uint32_t requestId);
    TransferEndReason precheckFault(const TransferInputs &inputs, float &targetLevelCm) const;
    TransferEndReason runningFault(const TransferInputs &inputs) const;
    void finish(TransferState terminalState, TransferEndReason reason);
    TransferOutputs makeOutputs() const;
};

} // namespace HWC

#endif
