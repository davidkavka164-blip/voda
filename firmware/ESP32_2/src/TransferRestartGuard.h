#ifndef HWC_TRANSFER_RESTART_GUARD_H
#define HWC_TRANSFER_RESTART_GUARD_H

#include <stdint.h>
#include "TransferLogic.h"

namespace HWC {

enum TransferJournalState : uint8_t {
    TRANSFER_JOURNAL_CLEAR = 0,
    TRANSFER_JOURNAL_PREPARED = 1,
    TRANSFER_JOURNAL_ACTIVE = 2,
    TRANSFER_JOURNAL_INTERRUPTED = 3,
    TRANSFER_JOURNAL_RESULT_PENDING = 4
};

enum TransferJournalLoadResult : uint8_t {
    TRANSFER_JOURNAL_LOAD_EMPTY,
    TRANSFER_JOURNAL_LOAD_VALID,
    TRANSFER_JOURNAL_LOAD_CORRUPT,
    TRANSFER_JOURNAL_LOAD_ERROR
};

enum TransferRestartGuardState : uint8_t {
    TRANSFER_RESTART_LOCKED = 0,
    TRANSFER_RESTART_READY = 1,
    TRANSFER_RESTART_PREPARED = 2,
    TRANSFER_RESTART_ACTIVE = 3,
    TRANSFER_RESTART_INTERRUPTED = 4,
    TRANSFER_RESTART_RESULT_PENDING = 5
};

struct TransferJournalRecord {
    uint32_t magic;
    uint16_t version;
    uint8_t state;
    uint8_t source;
    uint32_t requestId;
    float requestedLiters;
    float startLowerLevelCm;
    float targetLowerLevelCm;
    float estimatedTransferredLiters;
    uint8_t terminalState;
    uint8_t endReason;
    uint16_t reserved;
    uint32_t generation;
    uint32_t crc32;
};

class ITransferJournalStore {
public:
    virtual ~ITransferJournalStore() {}
    virtual TransferJournalLoadResult load(TransferJournalRecord& record) = 0;
    virtual bool save(const TransferJournalRecord& record) = 0;
};

class TransferRestartGuard {
public:
    TransferRestartGuard();
    bool restore(ITransferJournalStore& store);
    bool prepare(uint32_t requestId, TransferSource source, float requestedLiters);
    bool markActive(float startLowerLevelCm, float targetLowerLevelCm);
    bool markResult(TransferState terminalState, TransferEndReason reason,
                    float estimatedTransferredLiters);
    bool clearPrepared();
    bool acknowledgeRecovery();
    bool canStart() const;
    bool canOpenValve() const;
    TransferRestartGuardState state() const;
    const TransferJournalRecord& record() const;
    static uint32_t checksum(const TransferJournalRecord& record);
    static bool recordValid(const TransferJournalRecord& record);

private:
    ITransferJournalStore* store_;
    TransferJournalRecord record_;
    TransferRestartGuardState state_;
    bool persist(TransferJournalRecord next);
    static bool validRequest(uint32_t requestId, TransferSource source, float liters);
};

} // namespace HWC
#endif
