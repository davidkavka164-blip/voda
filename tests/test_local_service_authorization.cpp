#include "LocalServiceAuthorization.h"
#include <assert.h>
#include <stdio.h>
#include <stdint.h>

int main() {
    HWC::LocalServiceAuthorization gate;

    // A short press and contact bounce do not authorize service mode.
    gate.samplePhysicalInput(100U, true);
    gate.samplePhysicalInput(900U, false);
    gate.samplePhysicalInput(1200U, true);
    gate.samplePhysicalInput(4199U, true);
    assert(!gate.isArmed(4199U));
    gate.samplePhysicalInput(4200U, true);
    assert(gate.isArmed(4200U));

    // Authorization is single-use.
    assert(gate.consume(4201U));
    assert(!gate.consume(4202U));
    gate.samplePhysicalInput(4300U, true);
    assert(!gate.isArmed(4300U)); // must release first

    // Release permits a new deliberate hold.
    gate.samplePhysicalInput(4400U, false);
    gate.samplePhysicalInput(5000U, true);
    gate.samplePhysicalInput(8000U, true);
    assert(gate.isArmed(8000U));
    assert(!gate.consume(68001U)); // authorization window expired

    // The authorization expires at the exact 60-second boundary, not after it.
    HWC::LocalServiceAuthorization boundary;
    boundary.samplePhysicalInput(100U, true);
    boundary.samplePhysicalInput(3100U, true);
    assert(boundary.isArmed(3100U));
    assert(!boundary.consume(63100U));
    assert(!boundary.isArmed(63100U));

    // Unsigned elapsed arithmetic remains valid across millis() rollover.
    HWC::LocalServiceAuthorization wrap;
    wrap.samplePhysicalInput(0xFFFFFF00U, true);
    wrap.samplePhysicalInput(static_cast<uint32_t>(0xFFFFFF00U + 3000U), true);
    assert(wrap.isArmed(static_cast<uint32_t>(0xFFFFFF00U + 3000U)));
    assert(wrap.consume(static_cast<uint32_t>(0xFFFFFF00U + 3001U)));

    // A hold one millisecond short must never arm; reaching the exact
    // threshold on the next sample may arm.
    HWC::LocalServiceAuthorization shortHold;
    shortHold.samplePhysicalInput(10000U, true);
    shortHold.samplePhysicalInput(12999U, true);
    assert(!shortHold.isArmed(12999U));
    shortHold.samplePhysicalInput(13000U, true);
    assert(shortHold.isArmed(13000U));

    // A continuously held input cannot mint a second authorization after
    // the first one is consumed, even after the original window expires.
    assert(shortHold.consume(13001U));
    shortHold.samplePhysicalInput(14000U, true);
    shortHold.samplePhysicalInput(73000U, true);
    assert(!shortHold.isArmed(73000U));
    assert(!shortHold.consume(73000U));

    // Releasing and pressing again starts a fresh hold interval; elapsed
    // time from the previous press must not carry over.
    shortHold.samplePhysicalInput(73001U, false);
    shortHold.samplePhysicalInput(73002U, true);
    shortHold.samplePhysicalInput(76001U, true);
    assert(!shortHold.isArmed(76001U));
    shortHold.samplePhysicalInput(76002U, true);
    assert(shortHold.isArmed(76002U));

    // An authorization that expires across millis() rollover is also
    // rejected at the exact expiry boundary.
    HWC::LocalServiceAuthorization wrapExpiry;
    const uint32_t wrapStart = 0xFFFFFF00U;
    const uint32_t wrapArmed = static_cast<uint32_t>(wrapStart + 3000U);
    wrapExpiry.samplePhysicalInput(wrapStart, true);
    wrapExpiry.samplePhysicalInput(wrapArmed, true);
    const uint32_t wrapExpired = static_cast<uint32_t>(wrapArmed + 60000U);
    assert(!wrapExpiry.isArmed(wrapExpired));
    assert(!wrapExpiry.consume(wrapExpired));

    puts("PASS: local service authorization policy");
    return 0;
}
