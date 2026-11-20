#include "NvsTransferHistoryStore.h"
#include <assert.h>
#include <stdio.h>

bool Preferences::present = false;
bool Preferences::ignoreNextWrite = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
char Preferences::lastNamespace[32] = {0};

using namespace HWC;

static TransferHistorySnapshot makeSnapshot(uint32_t requestId, float liters) {
    TransferHistory h;
    TransferHistoryEntry e = {};
    e.requestId = requestId;
    e.source = TRANSFER_SOURCE_UPPER_TANK;
    e.terminalState = TRANSFER_STOPPED;
    e.endReason = TRANSFER_REASON_USER_STOP;
    e.requestedLiters = liters;
    e.estimatedTransferredLiters = liters * 0.4f;
    e.targetLowerLevelCm = 75.0f;
    e.finishedAtUptimeMs = 9000U + requestId;
    assert(h.append(e));
    return h.snapshot();
}

int main() {
    Preferences::clearForTest();
    NvsTransferHistoryStore store;
    assert(store.begin());
    assert(!store.hasRecord());

    const TransferHistorySnapshot first = makeSnapshot(77U, 250.0f);
    const TransferHistorySnapshot second = makeSnapshot(78U, 300.0f);
    assert(store.save(first));

    // Backend reports a full write but silently retains the previous valid blob.
    // save() must not report success for data that was not actually persisted.
    Preferences::ignoreNextWrite = true;
    assert(!store.save(second));

    TransferHistorySnapshot got = {};
    assert(store.load(got));
    assert(got.count == first.count);
    assert(got.entries[0].requestId == 77U);
    assert(got.entries[0].requestedLiters == 250.0f);

    // Corrupt persisted bytes must still be rejected and never overwritten.
    Preferences::storage[0] ^= 0x80U;
    assert(!store.load(got));
    assert(!store.save(first));
    puts("PASS: transfer history exact NVS read-back and corrupt-record protection");
    return 0;
}
