#include "PumpSafetyRelayOutput.h"
#include <assert.h>
#include <stdio.h>

struct Event { uint8_t pin; bool high; };
static Event events[32];
static unsigned count = 0;
static void writer(uint8_t pin, bool high) {
    assert(count < sizeof(events) / sizeof(events[0]));
    events[count++] = {pin, high};
}

int main() {
    HWC::PumpSafetyRelayOutput output;
    assert(!output.ready());
    assert(!output.applyPermit(true));
    assert(!output.begin(25, true, true, 0));
    assert(!output.begin(0, true, true, writer));

    // Commissioning gate is OFF: even a valid logical permit cannot energize the relay.
    count = 0;
    assert(output.begin(25, true, false, writer));
    assert(count == 1 && events[0].pin == 25 && !events[0].high);
    assert(output.ready() && !output.energized() && !output.commissioningEnabled());
    count = 0;
    assert(output.applyPermit(true));
    assert(count == 0 && !output.energized());
    assert(output.applyPermit(false));
    assert(count == 0 && !output.energized());

    // Once explicitly commissioned, HIGH means energize/permit; false always de-energizes.
    HWC::PumpSafetyRelayOutput commissioned;
    count = 0;
    assert(commissioned.begin(32, true, true, writer));
    assert(count == 1 && !events[0].high);
    count = 0;
    assert(commissioned.applyPermit(true));
    assert(count == 1 && events[0].pin == 32 && events[0].high);
    assert(commissioned.energized());
    count = 0;
    assert(commissioned.applyPermit(true));
    assert(count == 0); // no chatter
    assert(commissioned.applyPermit(false));
    assert(count == 1 && events[0].pin == 32 && !events[0].high);
    assert(!commissioned.energized());

    // Active-low hardware is also supported, with the inactive level HIGH.
    HWC::PumpSafetyRelayOutput activeLow;
    count = 0;
    assert(activeLow.begin(25, false, true, writer));
    assert(count == 1 && events[0].high);
    count = 0;
    assert(activeLow.applyPermit(true));
    assert(count == 1 && !events[0].high);
    assert(activeLow.applyPermit(false));
    assert(count == 2 && events[1].high);

    assert(commissioned.forceOff());
    assert(!commissioned.energized());
    puts("PASS: pump safety relay output commissioning gate and fail-off mapping");
    return 0;
}
