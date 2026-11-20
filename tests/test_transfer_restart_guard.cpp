#include "../firmware/common/TransferRestartGuard.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace HWC;
class FakeStore : public ITransferJournalStore {
public:
    FakeStore() : present(false), failSave(false) { memset(&record, 0, sizeof(record)); }
    TransferJournalLoadResult load(TransferJournalRecord& out) { if (!present) return TRANSFER_JOURNAL_LOAD_EMPTY; out=record; return TransferRestartGuard::recordValid(out) ? TRANSFER_JOURNAL_LOAD_VALID : TRANSFER_JOURNAL_LOAD_CORRUPT; }
    bool save(const TransferJournalRecord& in) { if (failSave || !TransferRestartGuard::recordValid(in)) return false; record=in; present=true; return true; }
    bool present, failSave; TransferJournalRecord record;
};
int main() {
    FakeStore store;
    TransferRestartGuard guard;
    assert(guard.restore(store)); assert(guard.canStart()); assert(!guard.canOpenValve());
    assert(guard.prepare(101, TRANSFER_SOURCE_WELL, 300.0f));
    assert(!guard.canStart() && !guard.canOpenValve());

    // Power loss during PREPARED: reboot converts it to INTERRUPTED and never resumes.
    TransferRestartGuard rebooted;
    assert(rebooted.restore(store));
    assert(rebooted.state() == TRANSFER_RESTART_INTERRUPTED);
    assert(!rebooted.canStart() && !rebooted.canOpenValve());
    assert(rebooted.record().requestId == 101);
    assert(rebooted.acknowledgeRecovery()); assert(rebooted.canStart());

    assert(rebooted.prepare(102, TRANSFER_SOURCE_UPPER_TANK, 500.0f));
    assert(rebooted.markActive(60.0f, 68.12f));
    assert(rebooted.canOpenValve());
    TransferRestartGuard rebootedDuringRun;
    assert(rebootedDuringRun.restore(store));
    assert(rebootedDuringRun.state() == TRANSFER_RESTART_INTERRUPTED);
    assert(!rebootedDuringRun.canOpenValve());
    assert(rebootedDuringRun.record().requestId == 102);
    assert(rebootedDuringRun.record().targetLowerLevelCm > 68.0f);

    assert(rebootedDuringRun.acknowledgeRecovery());
    assert(rebootedDuringRun.prepare(103, TRANSFER_SOURCE_WELL, 100.0f));
    assert(rebootedDuringRun.markActive(70.0f, 71.62f));
    assert(rebootedDuringRun.markResult(TRANSFER_ABORTED, TRANSFER_REASON_TIMEOUT, 30.0f));
    TransferRestartGuard pendingResult;
    assert(pendingResult.restore(store));
    assert(pendingResult.state() == TRANSFER_RESTART_RESULT_PENDING);
    assert(!pendingResult.canStart() && !pendingResult.canOpenValve());
    assert(pendingResult.acknowledgeRecovery()); assert(pendingResult.canStart());

    assert(pendingResult.prepare(104, TRANSFER_SOURCE_WELL, 50.0f));
    assert(pendingResult.clearPrepared()); assert(pendingResult.canStart());
    assert(!pendingResult.prepare(0, TRANSFER_SOURCE_WELL, 20.0f));

    assert(pendingResult.prepare(105, TRANSFER_SOURCE_WELL, 50.0f));
    store.failSave = true;
    assert(!pendingResult.markActive(70.0f, 70.81f));
    assert(pendingResult.state() == TRANSFER_RESTART_LOCKED);
    assert(!pendingResult.canStart() && !pendingResult.canOpenValve());

    FakeStore corrupt;
    corrupt.present = true; corrupt.record.magic = 0x1234;
    TransferRestartGuard failClosed;
    assert(!failClosed.restore(corrupt));
    assert(failClosed.state() == TRANSFER_RESTART_LOCKED);
    puts("PASS: transfer restart journal fail-closed recovery state machine");
}
