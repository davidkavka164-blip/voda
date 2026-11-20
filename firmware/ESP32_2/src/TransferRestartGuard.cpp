#include "TransferRestartGuard.h"
#include <math.h>
#include "WaterMath.h"
#include <string.h>

namespace HWC {
namespace {
const uint32_t JOURNAL_MAGIC = 0x4857544AUL; // HWTJ
const uint16_t JOURNAL_VERSION = 1U;
uint32_t crcByte(uint32_t c, uint8_t b) {
    c ^= b;
    for (uint8_t i = 0; i < 8U; ++i) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(c & 1U)));
        c = (c >> 1U) ^ (0xEDB88320UL & mask);
    }
    return c;
}
template <typename T> uint32_t crcValue(uint32_t c, const T& value) {
    const uint8_t* bytes = reinterpret_cast<const uint8_t*>(&value);
    for (size_t i = 0; i < sizeof(T); ++i) c = crcByte(c, bytes[i]);
    return c;
}
bool finiteValue(float value) { return isfinite(value) != 0; }
}

TransferRestartGuard::TransferRestartGuard()
    : store_(0), record_(), state_(TRANSFER_RESTART_LOCKED) {
    memset(&record_, 0, sizeof(record_));
}

uint32_t TransferRestartGuard::checksum(const TransferJournalRecord& r) {
    uint32_t c = 0xFFFFFFFFUL;
    c = crcValue(c, r.magic); c = crcValue(c, r.version);
    c = crcValue(c, r.state); c = crcValue(c, r.source);
    c = crcValue(c, r.requestId); c = crcValue(c, r.requestedLiters);
    c = crcValue(c, r.startLowerLevelCm); c = crcValue(c, r.targetLowerLevelCm);
    c = crcValue(c, r.estimatedTransferredLiters); c = crcValue(c, r.terminalState);
    c = crcValue(c, r.endReason); c = crcValue(c, r.reserved);
    c = crcValue(c, r.generation);
    return c ^ 0xFFFFFFFFUL;
}

bool TransferRestartGuard::recordValid(const TransferJournalRecord& r) {
    if (r.magic != JOURNAL_MAGIC || r.version != JOURNAL_VERSION ||
        r.state > TRANSFER_JOURNAL_RESULT_PENDING || r.reserved != 0U ||
        r.crc32 != checksum(r)) return false;
    if (r.state == TRANSFER_JOURNAL_CLEAR) {
        return r.requestId == 0U && r.source == TRANSFER_SOURCE_NONE &&
            r.requestedLiters == 0.0f && r.startLowerLevelCm == 0.0f &&
            r.targetLowerLevelCm == 0.0f && r.estimatedTransferredLiters == 0.0f &&
            r.terminalState == 0U && r.endReason == 0U;
    }
    if (r.requestId == 0U ||
        (r.source != TRANSFER_SOURCE_WELL && r.source != TRANSFER_SOURCE_UPPER_TANK) ||
        !finiteValue(r.requestedLiters) || r.requestedLiters <= 0.0f || r.requestedLiters > 7000.0f ||
        !finiteValue(r.startLowerLevelCm) || r.startLowerLevelCm < 0.0f ||
        r.startLowerLevelCm > LOWER_TANK_MAX_LEVEL_CM ||
        !finiteValue(r.targetLowerLevelCm) || r.targetLowerLevelCm < 0.0f ||
        r.targetLowerLevelCm > LOWER_TANK_MAX_LEVEL_CM ||
        !finiteValue(r.estimatedTransferredLiters) || r.estimatedTransferredLiters < 0.0f ||
        r.estimatedTransferredLiters > 100000.0f) return false;
    if (r.state == TRANSFER_JOURNAL_RESULT_PENDING) {
        return (r.terminalState == TRANSFER_COMPLETED || r.terminalState == TRANSFER_STOPPED ||
                r.terminalState == TRANSFER_ABORTED) && r.endReason <= TRANSFER_REASON_TIMEOUT;
    }
    return r.terminalState == 0U && r.endReason == 0U;
}

bool TransferRestartGuard::persist(TransferJournalRecord next) {
    if (!store_) return false;
    next.magic = JOURNAL_MAGIC;
    next.version = JOURNAL_VERSION;
    next.generation = record_.generation + 1U;
    next.reserved = 0U;
    next.crc32 = checksum(next);
    if (!recordValid(next) || !store_->save(next)) return false;
    TransferJournalRecord verify = {};
    if (store_->load(verify) != TRANSFER_JOURNAL_LOAD_VALID ||
        !recordValid(verify) || verify.generation != next.generation ||
        verify.crc32 != next.crc32) return false;
    record_ = verify;
    return true;
}

bool TransferRestartGuard::restore(ITransferJournalStore& store) {
    store_ = &store;
    TransferJournalRecord loaded = {};
    const TransferJournalLoadResult result = store.load(loaded);
    if (result == TRANSFER_JOURNAL_LOAD_EMPTY) {
        memset(&record_, 0, sizeof(record_));
        record_.magic = JOURNAL_MAGIC; record_.version = JOURNAL_VERSION;
        record_.state = TRANSFER_JOURNAL_CLEAR; record_.crc32 = checksum(record_);
        state_ = TRANSFER_RESTART_READY;
        return true;
    }
    if (result != TRANSFER_JOURNAL_LOAD_VALID || !recordValid(loaded)) {
        state_ = TRANSFER_RESTART_LOCKED;
        return false;
    }
    record_ = loaded;
    if (loaded.state == TRANSFER_JOURNAL_CLEAR) {
        state_ = TRANSFER_RESTART_READY;
        return true;
    }
    if (loaded.state == TRANSFER_JOURNAL_RESULT_PENDING) {
        state_ = TRANSFER_RESTART_RESULT_PENDING;
        return true;
    }
    if (loaded.state == TRANSFER_JOURNAL_INTERRUPTED) {
        state_ = TRANSFER_RESTART_INTERRUPTED;
        return true; // Do not rewrite flash on every reboot while awaiting operator acknowledgement.
    }
    // PREPARED and ACTIVE are both uncertain after reboot. Never resume them.
    TransferJournalRecord interrupted = loaded;
    interrupted.state = TRANSFER_JOURNAL_INTERRUPTED;
    if (!persist(interrupted)) {
        state_ = TRANSFER_RESTART_LOCKED;
        return false;
    }
    state_ = TRANSFER_RESTART_INTERRUPTED;
    return true;
}

bool TransferRestartGuard::validRequest(uint32_t id, TransferSource source, float liters) {
    return id != 0U && (source == TRANSFER_SOURCE_WELL || source == TRANSFER_SOURCE_UPPER_TANK) &&
           finiteValue(liters) && liters > 0.0f && liters <= 7000.0f;
}

bool TransferRestartGuard::prepare(uint32_t id, TransferSource source, float liters) {
    if (state_ != TRANSFER_RESTART_READY || !validRequest(id, source, liters)) return false;
    TransferJournalRecord next = {};
    next.state = TRANSFER_JOURNAL_PREPARED;
    next.requestId = id; next.source = static_cast<uint8_t>(source);
    next.requestedLiters = liters;
    if (!persist(next)) { state_ = TRANSFER_RESTART_LOCKED; return false; }
    state_ = TRANSFER_RESTART_PREPARED;
    return true;
}

bool TransferRestartGuard::markActive(float start, float target) {
    if (state_ != TRANSFER_RESTART_PREPARED || !finiteValue(start) || !finiteValue(target) ||
        start < 0.0f || start > LOWER_TANK_MAX_LEVEL_CM ||
        target < start || target > LOWER_TANK_MAX_LEVEL_CM) return false;
    TransferJournalRecord next = record_;
    next.state = TRANSFER_JOURNAL_ACTIVE;
    next.startLowerLevelCm = start; next.targetLowerLevelCm = target;
    if (!persist(next)) { state_ = TRANSFER_RESTART_LOCKED; return false; }
    state_ = TRANSFER_RESTART_ACTIVE;
    return true;
}

bool TransferRestartGuard::markResult(TransferState terminal, TransferEndReason reason, float estimate) {
    if ((state_ != TRANSFER_RESTART_ACTIVE && state_ != TRANSFER_RESTART_PREPARED) ||
        (terminal != TRANSFER_COMPLETED && terminal != TRANSFER_STOPPED && terminal != TRANSFER_ABORTED) ||
        reason > TRANSFER_REASON_TIMEOUT || !finiteValue(estimate) || estimate < 0.0f || estimate > 100000.0f)
        return false;
    TransferJournalRecord next = record_;
    next.state = TRANSFER_JOURNAL_RESULT_PENDING;
    next.terminalState = static_cast<uint8_t>(terminal);
    next.endReason = static_cast<uint8_t>(reason);
    next.estimatedTransferredLiters = estimate;
    if (!persist(next)) { state_ = TRANSFER_RESTART_LOCKED; return false; }
    state_ = TRANSFER_RESTART_RESULT_PENDING;
    return true;
}

bool TransferRestartGuard::clearPrepared() {
    if (state_ != TRANSFER_RESTART_PREPARED) return false;
    TransferJournalRecord next = {};
    next.state = TRANSFER_JOURNAL_CLEAR;
    if (!persist(next)) { state_ = TRANSFER_RESTART_LOCKED; return false; }
    state_ = TRANSFER_RESTART_READY;
    return true;
}

bool TransferRestartGuard::acknowledgeRecovery() {
    if (state_ != TRANSFER_RESTART_INTERRUPTED && state_ != TRANSFER_RESTART_RESULT_PENDING) return false;
    TransferJournalRecord next = {};
    next.state = TRANSFER_JOURNAL_CLEAR;
    if (!persist(next)) { state_ = TRANSFER_RESTART_LOCKED; return false; }
    state_ = TRANSFER_RESTART_READY;
    return true;
}

bool TransferRestartGuard::canStart() const { return state_ == TRANSFER_RESTART_READY; }
bool TransferRestartGuard::canOpenValve() const { return state_ == TRANSFER_RESTART_ACTIVE; }
TransferRestartGuardState TransferRestartGuard::state() const { return state_; }
const TransferJournalRecord& TransferRestartGuard::record() const { return record_; }

} // namespace HWC
