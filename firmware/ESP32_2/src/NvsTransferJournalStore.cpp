#include "NvsTransferJournalStore.h"
#include <string.h>
namespace HWC {
namespace {
const char* keyFor(uint8_t slot) { return slot == 0U ? "slot0" : "slot1"; }
bool sameRecord(const TransferJournalRecord& a, const TransferJournalRecord& b) {
    return a.magic == b.magic && a.version == b.version && a.state == b.state &&
        a.source == b.source && a.requestId == b.requestId &&
        a.requestedLiters == b.requestedLiters && a.startLowerLevelCm == b.startLowerLevelCm &&
        a.targetLowerLevelCm == b.targetLowerLevelCm &&
        a.estimatedTransferredLiters == b.estimatedTransferredLiters &&
        a.terminalState == b.terminalState && a.endReason == b.endReason &&
        a.reserved == b.reserved && a.generation == b.generation && a.crc32 == b.crc32;
}
}
NvsTransferJournalStore::NvsTransferJournalStore(const char* ns) : nameSpace_(ns), opened_(false) {}
bool NvsTransferJournalStore::begin() {
    if (opened_) return true;
    opened_ = nameSpace_ && nameSpace_[0] && strlen(nameSpace_) <= 15U && preferences_.begin(nameSpace_, false);
    return opened_;
}
void NvsTransferJournalStore::end() { if (opened_) preferences_.end(); opened_ = false; }
bool NvsTransferJournalStore::isOpen() const { return opened_; }
bool NvsTransferJournalStore::newerGeneration(uint32_t a, uint32_t b) { return static_cast<int32_t>(a - b) > 0; }
TransferJournalLoadResult NvsTransferJournalStore::readSlot(uint8_t slot, TransferJournalRecord& out) {
    memset(&out, 0, sizeof(out));
    if (!opened_ || slot > 1U) return TRANSFER_JOURNAL_LOAD_ERROR;
    const char* key = keyFor(slot);
    if (!preferences_.isKey(key)) return TRANSFER_JOURNAL_LOAD_EMPTY;
    if (preferences_.getBytesLength(key) != sizeof(out)) return TRANSFER_JOURNAL_LOAD_CORRUPT;
    if (preferences_.getBytes(key, &out, sizeof(out)) != sizeof(out)) return TRANSFER_JOURNAL_LOAD_ERROR;
    return TransferRestartGuard::recordValid(out) ? TRANSFER_JOURNAL_LOAD_VALID : TRANSFER_JOURNAL_LOAD_CORRUPT;
}
TransferJournalLoadResult NvsTransferJournalStore::load(TransferJournalRecord& out) {
    memset(&out, 0, sizeof(out));
    if (!opened_) return TRANSFER_JOURNAL_LOAD_ERROR;
    TransferJournalRecord a = {}, b = {};
    const TransferJournalLoadResult ra = readSlot(0U, a), rb = readSlot(1U, b);
    if (ra == TRANSFER_JOURNAL_LOAD_ERROR || rb == TRANSFER_JOURNAL_LOAD_ERROR) return TRANSFER_JOURNAL_LOAD_ERROR;
    // A corrupt slot may contain the newest committed transfer state. Falling
    // back to the older slot could erase an interrupted-transfer lockout.
    if (ra == TRANSFER_JOURNAL_LOAD_CORRUPT || rb == TRANSFER_JOURNAL_LOAD_CORRUPT)
        return TRANSFER_JOURNAL_LOAD_CORRUPT;
    if (ra == TRANSFER_JOURNAL_LOAD_VALID && rb == TRANSFER_JOURNAL_LOAD_VALID) {
        if (a.generation == b.generation) {
            if (!sameRecord(a, b)) return TRANSFER_JOURNAL_LOAD_CORRUPT;
            out = a;
        } else out = newerGeneration(b.generation, a.generation) ? b : a;
        return TRANSFER_JOURNAL_LOAD_VALID;
    }
    if (ra == TRANSFER_JOURNAL_LOAD_VALID) { out = a; return ra; }
    if (rb == TRANSFER_JOURNAL_LOAD_VALID) { out = b; return rb; }
    if (ra == TRANSFER_JOURNAL_LOAD_CORRUPT || rb == TRANSFER_JOURNAL_LOAD_CORRUPT) return TRANSFER_JOURNAL_LOAD_CORRUPT;
    return TRANSFER_JOURNAL_LOAD_EMPTY;
}
bool NvsTransferJournalStore::save(const TransferJournalRecord& input) {
    if (!opened_ || !TransferRestartGuard::recordValid(input)) return false;
    TransferJournalRecord current = {};
    const TransferJournalLoadResult currentState = load(current);
    if (currentState == TRANSFER_JOURNAL_LOAD_CORRUPT || currentState == TRANSFER_JOURNAL_LOAD_ERROR) return false;
    uint8_t target = 0U;
    uint32_t generation = 1U;
    if (currentState == TRANSFER_JOURNAL_LOAD_VALID) {
        TransferJournalRecord a = {}, b = {};
        const TransferJournalLoadResult ra = readSlot(0U, a), rb = readSlot(1U, b);
        const uint8_t active = (ra == TRANSFER_JOURNAL_LOAD_VALID &&
            (rb != TRANSFER_JOURNAL_LOAD_VALID || !newerGeneration(b.generation, a.generation))) ? 0U : 1U;
        target = static_cast<uint8_t>(1U - active);
        generation = current.generation + 1U;
    }
    TransferJournalRecord next = input;
    next.generation = generation;
    next.crc32 = TransferRestartGuard::checksum(next);
    if (!TransferRestartGuard::recordValid(next) ||
        preferences_.putBytes(keyFor(target), &next, sizeof(next)) != sizeof(next)) return false;
    TransferJournalRecord verify = {};
    return load(verify) == TRANSFER_JOURNAL_LOAD_VALID && sameRecord(verify, next);
}
}
