#include "../firmware/common/LowerTankInflowWatchdog.h"
#include <assert.h>
#include <math.h>

using namespace HWC;

int main() {
    LowerTankInflowWatchdogConfig cfg = {10000U, 5000U, 5.0f};
    LowerTankInflowWatchdog w;

    // Invalid configuration fails closed.
    LowerTankInflowWatchdogConfig bad = {0U, 5000U, 5.0f};
    assert(!w.begin(1000U, 1000.0f, bad));
    assert(w.status() == LOWER_INFLOW_INVALID_CONFIG);

    // No initial inflow within the startup grace period is a terminal fault.
    assert(w.begin(1000U, 1000.0f, cfg));
    assert(w.update(5000U, 1002.0f, true) == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS);
    assert(w.update(11000U, 1003.0f, true) == LOWER_INFLOW_NO_PROGRESS);
    assert(w.update(12000U, 1020.0f, true) == LOWER_INFLOW_NO_PROGRESS); // terminal latch

    // Progress observed exactly at the startup deadline is still accepted;
    // subsequent stall is detected independently.
    assert(w.begin(20000U, 1000.0f, cfg));
    assert(w.update(30000U, 1006.0f, true) == LOWER_INFLOW_CONFIRMED);
    assert(fabsf(w.observedIncreaseLiters() - 6.0f) < 0.01f);
    assert(w.update(34000U, 1008.0f, true) == LOWER_INFLOW_CONFIRMED); // below deadband
    assert(w.update(35000U, 1012.0f, true) == LOWER_INFLOW_CONFIRMED); // new progress
    assert(w.update(39999U, 1012.0f, true) == LOWER_INFLOW_CONFIRMED);
    assert(w.update(40000U, 1012.0f, true) == LOWER_INFLOW_NO_PROGRESS);

    // First progress observed after the startup deadline is rejected, even if
    // the current sample shows a large increase.
    assert(w.begin(50000U, 1000.0f, cfg));
    assert(w.update(60001U, 1020.0f, true) == LOWER_INFLOW_NO_PROGRESS);

    // Falling level and small sensor noise do not count as progress.
    assert(w.begin(40000U, 1000.0f, cfg));
    assert(w.update(41000U, 1004.9f, true) == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS);
    assert(w.update(42000U, 999.0f, true) == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS);
    assert(w.update(50000U, 1001.0f, true) == LOWER_INFLOW_NO_PROGRESS);

    // A stale/invalid sample is a terminal fault, never silently ignored.
    assert(w.begin(60000U, 1000.0f, cfg));
    assert(w.update(61000U, 1006.0f, true) == LOWER_INFLOW_CONFIRMED);
    assert(w.update(62000U, NAN, true) == LOWER_INFLOW_INVALID_SAMPLE);
    assert(w.update(63000U, 1020.0f, true) == LOWER_INFLOW_INVALID_SAMPLE);

    // Millisecond rollover is handled using unsigned elapsed-time arithmetic.
    assert(w.begin(0xFFFFFF00U, 1000.0f, cfg));
    assert(w.update(0x00000100U, 1001.0f, true) == LOWER_INFLOW_WAITING_FOR_FIRST_PROGRESS);
    assert(w.update(0x00002710U, 1001.0f, true) == LOWER_INFLOW_NO_PROGRESS);

    return 0;
}
