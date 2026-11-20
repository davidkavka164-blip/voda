#include "../firmware/common/TransferHistory.h"
#include <assert.h>
#include <stdio.h>
using namespace HWC;
static TransferHistoryEntry entry(uint32_t id) { TransferHistoryEntry e={}; e.requestId=id; e.source=TRANSFER_SOURCE_WELL; e.terminalState=TRANSFER_COMPLETED; e.endReason=TRANSFER_REASON_TARGET_REACHED; e.requestedLiters=100; e.estimatedTransferredLiters=98; e.targetLowerLevelCm=80; e.finishedAtUptimeMs=id*100; return e; }
int main(){
 TransferHistory h; assert(isTransferHistorySnapshotValid(h.snapshot()));
 for(uint32_t i=1;i<=TRANSFER_HISTORY_CAPACITY;++i) assert(h.append(entry(i)));
 assert(h.snapshot().count==TRANSFER_HISTORY_CAPACITY && h.snapshot().nextIndex==0);
 assert(!h.append(entry(1)));
 assert(h.append(entry(13))); assert(h.snapshot().nextIndex==1); assert(isTransferHistorySnapshotValid(h.snapshot()));
 TransferHistory restored; assert(restored.restore(h.snapshot()));
 TransferHistoryEntry bad=entry(14); bad.estimatedTransferredLiters=-1; assert(!restored.append(bad));
 TransferHistorySnapshot corrupt=h.snapshot(); corrupt.count=250; assert(!restored.restore(corrupt));
 // Merge a RAM-only pending history into an older persisted history; duplicates are ignored.
 TransferHistory base; assert(base.append(entry(100))); assert(base.append(entry(101)));
 TransferHistory pending; assert(pending.append(entry(101))); assert(pending.append(entry(102)));
 TransferHistorySnapshot merged = {};
 assert(mergeTransferHistorySnapshots(base.snapshot(), pending.snapshot(), merged));
 assert(merged.count == 3);
 assert(merged.entries[0].requestId == 100 && merged.entries[1].requestId == 101 && merged.entries[2].requestId == 102);
 TransferHistorySnapshot invalidPending = pending.snapshot(); invalidPending.entries[0].requestId = 0;
 assert(!mergeTransferHistorySnapshots(base.snapshot(), invalidPending, merged));
 puts("PASS: transfer history ring, validation, duplicate protection and recovery merge");
}
