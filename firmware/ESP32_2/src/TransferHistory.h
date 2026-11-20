#ifndef HWC_TRANSFER_HISTORY_H
#define HWC_TRANSFER_HISTORY_H
#include "TransferLogic.h"
#include <stddef.h>
#include <stdint.h>
namespace HWC {
static const size_t TRANSFER_HISTORY_CAPACITY = 12;
struct TransferHistoryEntry {
    uint32_t requestId;
    uint8_t source;
    uint8_t terminalState;
    uint8_t endReason;
    uint8_t reserved;
    float requestedLiters;
    float estimatedTransferredLiters;
    float targetLowerLevelCm;
    uint32_t finishedAtUptimeMs;
};
struct TransferHistorySnapshot {
    uint8_t count;
    uint8_t nextIndex;
    uint8_t reserved[2];
    TransferHistoryEntry entries[TRANSFER_HISTORY_CAPACITY];
};
TransferHistorySnapshot emptyTransferHistory();
bool isTransferHistoryEntryValid(const TransferHistoryEntry& entry);
bool isTransferHistorySnapshotValid(const TransferHistorySnapshot& snapshot);
bool mergeTransferHistorySnapshots(const TransferHistorySnapshot& base, const TransferHistorySnapshot& pending, TransferHistorySnapshot& merged);
class TransferHistory {
public:
    TransferHistory();
    bool restore(const TransferHistorySnapshot& snapshot);
    bool append(const TransferHistoryEntry& entry);
    const TransferHistorySnapshot& snapshot() const;
private:
    TransferHistorySnapshot data_;
};
}
#endif
