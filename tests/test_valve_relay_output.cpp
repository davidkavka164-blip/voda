#include "ValveRelayOutput.h"
#include <assert.h>
#include <stdio.h>

struct WriteEvent { uint8_t pin; bool high; };
static WriteEvent events[32];
static unsigned eventCount = 0;

static void writer(uint8_t pin, bool high) {
    assert(eventCount < sizeof(events) / sizeof(events[0]));
    events[eventCount].pin = pin;
    events[eventCount].high = high;
    ++eventCount;
}

int main() {
    HWC::ValveRelayOutput output;
    assert(!output.ready());
    assert(!output.apply(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER));

    assert(!output.begin(16, 16, true, writer)); // same relay pin is invalid
    assert(!output.ready());
    assert(!output.begin(16, 17, true, 0)); // missing hardware writer is invalid

    assert(output.begin(16, 17, true, writer));
    assert(output.ready());
    assert(output.appliedCommand() == HWC::MANUAL_VALVES_CLOSE_ALL);
    assert(!output.wellRelayEnergized() && !output.upperRelayEnergized());
    assert(eventCount == 2);
    assert(events[0].pin == 16 && !events[0].high);
    assert(events[1].pin == 17 && !events[1].high);

    eventCount = 0;
    assert(output.apply(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER));
    assert(eventCount == 3); // both OFF first, then well relay ON
    assert(events[0].pin == 16 && !events[0].high);
    assert(events[1].pin == 17 && !events[1].high);
    assert(events[2].pin == 16 && events[2].high);
    assert(output.wellRelayEnergized() && !output.upperRelayEnergized());

    eventCount = 0;
    assert(output.apply(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER));
    assert(eventCount == 0); // no relay chatter for an unchanged command

    assert(output.forceClose());
    assert(eventCount == 2);
    assert(!output.wellRelayEnergized() && !output.upperRelayEnergized());

    eventCount = 0;
    assert(output.apply(HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER));
    assert(eventCount == 3);
    assert(events[0].pin == 16 && !events[0].high);
    assert(events[1].pin == 17 && !events[1].high);
    assert(events[2].pin == 17 && events[2].high);
    assert(!output.wellRelayEnergized() && output.upperRelayEnergized());

    eventCount = 0;
    assert(output.apply(static_cast<HWC::ManualValveCommand>(99)));
    assert(output.appliedCommand() == HWC::MANUAL_VALVES_CLOSE_ALL);
    assert(!output.wellRelayEnergized() && !output.upperRelayEnergized());
    assert(eventCount == 2);

    // Active-low configurations are also mapped to the de-energized level.
    HWC::ValveRelayOutput activeLow;
    eventCount = 0;
    assert(activeLow.begin(21, 22, false, writer));
    assert(events[0].pin == 21 && events[0].high);
    assert(events[1].pin == 22 && events[1].high);
    eventCount = 0;
    assert(activeLow.apply(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER));
    assert(events[0].pin == 21 && events[0].high);
    assert(events[1].pin == 22 && events[1].high);
    assert(events[2].pin == 21 && !events[2].high);

    puts("PASS: fail-safe valve relay mapping and break-before-make");
    return 0;
}
