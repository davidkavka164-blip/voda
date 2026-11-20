#include "NvsTransferJournalStore.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
unsigned char Preferences::data[2][256] = {}; size_t Preferences::lengths[2] = {}; bool Preferences::present[2] = {}; bool Preferences::failWrite = false; char Preferences::lastNamespace[32] = {};
using namespace HWC;
static TransferJournalRecord makeRecord(uint8_t state, uint32_t generation, uint32_t requestId=0U) {
    TransferJournalRecord r={};
    r.magic=0x4857544AUL; r.version=1; r.state=state; r.generation=generation;
    if (state == TRANSFER_JOURNAL_PREPARED || state == TRANSFER_JOURNAL_ACTIVE ||
        state == TRANSFER_JOURNAL_INTERRUPTED || state == TRANSFER_JOURNAL_RESULT_PENDING) {
        r.source=TRANSFER_SOURCE_WELL; r.requestId=requestId; r.requestedLiters=200.0f;
    }
    if (state == TRANSFER_JOURNAL_ACTIVE || state == TRANSFER_JOURNAL_INTERRUPTED) {
        r.startLowerLevelCm=40.0f; r.targetLowerLevelCm=43.25f;
    }
    if (state == TRANSFER_JOURNAL_RESULT_PENDING) {
        r.terminalState=TRANSFER_ABORTED; r.endReason=TRANSFER_REASON_SYSTEM_NOT_READY;
    }
    r.crc32=TransferRestartGuard::checksum(r);
    return r;
}
int main() {
    Preferences::clearForTest();
    NvsTransferJournalStore store; assert(store.begin());
    TransferJournalRecord got={}; assert(store.load(got)==TRANSFER_JOURNAL_LOAD_EMPTY);
    assert(strcmp(Preferences::lastNamespace, "hwc_xfer_jrnl") == 0);
    TransferJournalRecord r=makeRecord(TRANSFER_JOURNAL_CLEAR, 1U); assert(store.save(r));
    assert(store.load(got)==TRANSFER_JOURNAL_LOAD_VALID && got.state==TRANSFER_JOURNAL_CLEAR);
    r.state=TRANSFER_JOURNAL_PREPARED; r.source=TRANSFER_SOURCE_WELL; r.requestId=55; r.requestedLiters=200; r.crc32=TransferRestartGuard::checksum(r);
    assert(store.save(r)); assert(store.load(got)==TRANSFER_JOURNAL_LOAD_VALID); assert(got.requestId==55 && got.state==TRANSFER_JOURNAL_PREPARED);
    // Any corrupt slot may be the newest committed state. Do not roll back
    // to an older CLEAR record, and do not overwrite evidence automatically.
    Preferences::data[1][0] ^= 0x40;
    assert(store.load(got)==TRANSFER_JOURNAL_LOAD_CORRUPT);
    r=makeRecord(TRANSFER_JOURNAL_PREPARED, 0U, 56U);
    assert(!store.save(r));
    assert(store.load(got)==TRANSFER_JOURNAL_LOAD_CORRUPT);
    // Generation comparison and next-generation write must survive uint32 wrap.
    Preferences::clearForTest();
    TransferJournalRecord nearWrap=makeRecord(TRANSFER_JOURNAL_CLEAR, 0xFFFFFFFEUL);
    TransferJournalRecord latest=makeRecord(TRANSFER_JOURNAL_CLEAR, 0xFFFFFFFFUL);
    memcpy(Preferences::data[0], &nearWrap, sizeof(nearWrap));
    Preferences::lengths[0]=sizeof(nearWrap); Preferences::present[0]=true;
    memcpy(Preferences::data[1], &latest, sizeof(latest));
    Preferences::lengths[1]=sizeof(latest); Preferences::present[1]=true;
    NvsTransferJournalStore wrapStore; assert(wrapStore.begin());
    assert(wrapStore.load(got)==TRANSFER_JOURNAL_LOAD_VALID && got.generation==0xFFFFFFFFUL);
    TransferJournalRecord wrapped=makeRecord(TRANSFER_JOURNAL_PREPARED, 0U, 77U);
    assert(wrapStore.save(wrapped));
    assert(wrapStore.load(got)==TRANSFER_JOURNAL_LOAD_VALID);
    assert(got.generation==0U && got.requestId==77U && got.state==TRANSFER_JOURNAL_PREPARED);

    // Exercise the actual guard across an NVS-backed reboot, not only a fake store.
    Preferences::clearForTest();
    NvsTransferJournalStore rebootStore; assert(rebootStore.begin());
    TransferRestartGuard beforeReset; assert(beforeReset.restore(rebootStore));
    assert(beforeReset.prepare(9001U, TRANSFER_SOURCE_UPPER_TANK, 400.0f));
    assert(beforeReset.markActive(55.0f, 61.49f));
    TransferRestartGuard afterReset; assert(afterReset.restore(rebootStore));
    assert(afterReset.state() == TRANSFER_RESTART_INTERRUPTED);
    assert(!afterReset.canOpenValve() && !afterReset.canStart());
    assert(afterReset.record().requestId == 9001U);
    const uint32_t interruptedGeneration = afterReset.record().generation;
    TransferRestartGuard repeatedBoot; assert(repeatedBoot.restore(rebootStore));
    assert(repeatedBoot.state() == TRANSFER_RESTART_INTERRUPTED);
    assert(repeatedBoot.record().generation == interruptedGeneration); // No flash write on every reboot.

    // A failed durable write locks the guard rather than permitting activation.
    assert(afterReset.acknowledgeRecovery());
    Preferences::failWrite = true;
    assert(!afterReset.prepare(9002U, TRANSFER_SOURCE_WELL, 100.0f));
    assert(!afterReset.canStart() && !afterReset.canOpenValve());

    puts("PASS: NVS transfer journal two-slot fallback and corruption detection");
}
