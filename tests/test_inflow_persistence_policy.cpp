#include "InflowPersistencePolicy.h"
#include <assert.h>
#include <stdint.h>
#include <stdio.h>

int main() {
    HWC::InflowPersistencePolicy p = HWC::defaultInflowPersistencePolicy();
    assert(!p.dirty);
    assert(!HWC::inflowPersistenceDue(p, true, 0));

    // First valid date marks the history dirty once, allowing 30-day pruning to persist.
    HWC::observeInflowPersistence(p, false, 20261029U, true);
    assert(p.dirty);
    assert(HWC::inflowPersistenceDue(p, true, 100U));
    assert(!HWC::inflowPersistenceDue(p, false, 100U));
    HWC::markInflowPersisted(p, 100U);
    assert(!p.dirty);
    assert(!HWC::inflowPersistenceDue(p, true, 299999U));
    assert(!HWC::inflowPersistenceDue(p, true, 300099U));

    // Stable samples do not trigger NVS writes.
    HWC::observeInflowPersistence(p, false, 20261029U, true);
    assert(!p.dirty);
    assert(!HWC::inflowPersistenceDue(p, true, 600000U));

    // Positive change marks dirty, but save is throttled until 5 minutes after last save.
    HWC::observeInflowPersistence(p, true, 20261029U, true);
    assert(p.dirty);
    assert(!HWC::inflowPersistenceDue(p, true, 300099U));
    assert(HWC::inflowPersistenceDue(p, true, 300100U));
    HWC::markInflowPersisted(p, 300100U);
    assert(!p.dirty);

    // A valid date rollover marks the snapshot dirty even without new inflow.
    HWC::observeInflowPersistence(p, false, 20261030U, true);
    assert(p.dirty);
    assert(!HWC::inflowPersistenceDue(p, true, 300101U));
    assert(HWC::inflowPersistenceDue(p, true, 600100U));

    // Invalid system time does not invent a date rollover or trigger a write by itself.
    HWC::markInflowPersisted(p, 600100U);
    HWC::observeInflowPersistence(p, false, 0, false);
    assert(!p.dirty);

    // Unsigned subtraction handles millis() rollover.
    HWC::observeInflowPersistence(p, true, 0, false);
    assert(p.dirty);
    HWC::markInflowPersisted(p, 0xFFFFFF00UL);
    HWC::observeInflowPersistence(p, true, 0, false);
    assert(!HWC::inflowPersistenceDue(p, true, 0x00000100UL));
    assert(HWC::inflowPersistenceDue(p, true, 0x000492E0UL));

    puts("PASS: InflowPersistencePolicy native tests");
    return 0;
}
