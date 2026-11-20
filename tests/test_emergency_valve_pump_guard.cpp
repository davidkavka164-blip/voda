#include "EmergencyValvePumpGuard.h"
#include <assert.h>
#include <stdio.h>

int main() {
    // Invalid/unavailable well sensor: emergency manual operation may proceed
    // after the operator's physical inspection and confirmation.
    assert(HWC::emergencyWellValvePumpGuardAllows(false, false));
    assert(HWC::emergencyWellValvePumpGuardAllows(false, true));

    // Valid sensor: preserve the existing dry-run pump permit.
    assert(HWC::emergencyWellValvePumpGuardAllows(true, true));
    assert(!HWC::emergencyWellValvePumpGuardAllows(true, false));

    puts("PASS: emergency well-valve route preserves valid-sensor dry-run pump permit");
    return 0;
}
