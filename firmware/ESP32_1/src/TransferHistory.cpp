#include "TransferHistory.h"
#include "WaterMath.h"
#include <math.h>
namespace HWC {
TransferHistorySnapshot emptyTransferHistory() { TransferHistorySnapshot s = {}; return s; }
bool isTransferHistoryEntryValid(const TransferHistoryEntry& e) {
    return e.requestId != 0 &&
        (e.source == TRANSFER_SOURCE_WELL || e.source == TRANSFER_SOURCE_UPPER_TANK) &&
        (e.terminalState == TRANSFER_COMPLETED || e.terminalState == TRANSFER_STOPPED || e.terminalState == TRANSFER_ABORTED) &&
        e.endReason <= TRANSFER_REASON_TIMEOUT && e.reserved == 0 && isfinite(e.requestedLiters) && e.requestedLiters > 0.0f &&
        isfinite(e.estimatedTransferredLiters) && e.estimatedTransferredLiters >= 0.0f &&
        e.estimatedTransferredLiters <= 100000.0f && isfinite(e.targetLowerLevelCm) &&
        e.targetLowerLevelCm >= 0.0f && e.targetLowerLevelCm <= LOWER_TANK_MAX_LEVEL_CM;
}
bool isTransferHistorySnapshotValid(const TransferHistorySnapshot& s) {
    if (s.count > TRANSFER_HISTORY_CAPACITY || s.nextIndex >= TRANSFER_HISTORY_CAPACITY || s.reserved[0] != 0 || s.reserved[1] != 0) return false;
    if ((s.count == 0 && s.nextIndex != 0) || (s.count > 0 && s.count < TRANSFER_HISTORY_CAPACITY && s.nextIndex != s.count) || (s.count == TRANSFER_HISTORY_CAPACITY && s.nextIndex >= TRANSFER_HISTORY_CAPACITY)) return false;
    for (size_t i = 0; i < s.count; ++i) if (!isTransferHistoryEntryValid(s.entries[i])) return false;
    for (size_t i = s.count; i < TRANSFER_HISTORY_CAPACITY; ++i) {
        const TransferHistoryEntry& e = s.entries[i];
        if (e.requestId != 0 || e.source != 0 || e.terminalState != 0 || e.endReason != 0 ||
            e.requestedLiters != 0.0f || e.estimatedTransferredLiters != 0.0f || e.targetLowerLevelCm != 0.0f || e.finishedAtUptimeMs != 0) return false;
    }
    return true;
}
bool mergeTransferHistorySnapshots(const TransferHistorySnapshot& base, const TransferHistorySnapshot& pending, TransferHistorySnapshot& merged) {
    if (!isTransferHistorySnapshotValid(base) || !isTransferHistorySnapshotValid(pending)) return false;
    TransferHistory combined;
    if (!combined.restore(base)) return false;
    const size_t firstPending = (static_cast<size_t>(pending.nextIndex) + TRANSFER_HISTORY_CAPACITY - pending.count) % TRANSFER_HISTORY_CAPACITY;
    for (size_t i = 0; i < pending.count; ++i) {
        const TransferHistoryEntry& candidate = pending.entries[(firstPending + i) % TRANSFER_HISTORY_CAPACITY];
        bool duplicate = false;
        const TransferHistorySnapshot& current = combined.snapshot();
        for (size_t j = 0; j < current.count; ++j) {
            const size_t firstCurrent = (static_cast<size_t>(current.nextIndex) + TRANSFER_HISTORY_CAPACITY - current.count) % TRANSFER_HISTORY_CAPACITY;
            if (current.entries[(firstCurrent + j) % TRANSFER_HISTORY_CAPACITY].requestId == candidate.requestId) {
                duplicate = true;
                break;
            }
        }
        if (!duplicate && !combined.append(candidate)) return false;
    }
    merged = combined.snapshot();
    return isTransferHistorySnapshotValid(merged);
}

TransferHistory::TransferHistory() : data_(emptyTransferHistory()) {}
bool TransferHistory::restore(const TransferHistorySnapshot& s) { if (!isTransferHistorySnapshotValid(s)) return false; data_ = s; return true; }
bool TransferHistory::append(const TransferHistoryEntry& e) {
    if (!isTransferHistoryEntryValid(e)) return false;
    // Reject an ID already present in the retained history.
    for (size_t i = 0; i < data_.count; ++i) if (data_.entries[i].requestId == e.requestId) return false;
    if (data_.count < TRANSFER_HISTORY_CAPACITY) {
        data_.entries[data_.count++] = e;
        data_.nextIndex = static_cast<uint8_t>(data_.count % TRANSFER_HISTORY_CAPACITY);
    } else {
        data_.entries[data_.nextIndex] = e;
        data_.nextIndex = static_cast<uint8_t>((data_.nextIndex + 1U) % TRANSFER_HISTORY_CAPACITY);
    }
    return true;
}
const TransferHistorySnapshot& TransferHistory::snapshot() const { return data_; }
}
