#include "NvsDailyInflowStore.h"
#include <Preferences.h>
#include <assert.h>
#include <math.h>
#include <stdio.h>

bool Preferences::present = false;
size_t Preferences::length = 0;
unsigned char Preferences::storage[4096] = {0};
char Preferences::lastNamespace[32] = {0};

static bool near(float a, float b) { return fabsf(a - b) < 0.01f; }

int main() {
    using namespace HWC;
    Preferences::clearForTest();
    DailyInflowTracker tracker;
    tracker.observe(100.0f, true, 20261028, true);
    tracker.observe(125.0f, true, 20261028, true);
    tracker.observe(110.0f, true, 20261028, true);
    tracker.observe(130.0f, true, 20261028, true);
    tracker.observe(500.0f, true, 0, false);
    tracker.observe(515.0f, true, 0, false);
    DailyInflowSnapshot snapshot = {};
    tracker.exportSnapshot(snapshot);

    NvsDailyInflowStore store;
    assert(!store.save(snapshot));
    assert(store.begin());
    assert(store.save(snapshot));
    store.end();

    NvsDailyInflowStore reopened;
    assert(reopened.begin());
    DailyInflowSnapshot loaded = {};
    assert(reopened.load(loaded));
    DailyInflowTracker restored;
    assert(restored.restoreSnapshot(loaded));
    assert(near(restored.litersForDate(20261028), 45.0f));
    assert(near(restored.undatedLiters(), 15.0f));

    // Invalid snapshot data and a corrupted persisted record must be rejected.
    loaded.undatedLiters = -1.0f;
    assert(!reopened.save(loaded));
    Preferences::storage[0] ^= 0x80U;
    DailyInflowSnapshot corrupted = {};
    assert(!reopened.load(corrupted));
    assert(!reopened.save(snapshot)); // do not silently overwrite corrupt history
    puts("PASS: NvsDailyInflowStore native stub tests");
    return 0;
}
