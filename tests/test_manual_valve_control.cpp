#include "ManualValveControl.h"
#include <assert.h>
#include <stdio.h>

int main() {
    HWC::ManualValveControl control;
    assert(!control.active());
    assert(control.command() == HWC::MANUAL_VALVES_CLOSE_ALL);
    assert(control.operatingMode() == HWC::MANUAL_VALVE_OPERATING_NONE);

    // Service mode is allowed only with healthy relevant sensors.
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_SERVICE, false, true, false, true) ==
           HWC::MANUAL_VALVE_SENSOR_FAULT_BLOCKS_SERVICE);
    assert(!control.active());
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_SERVICE, false, false, true, true) ==
           HWC::MANUAL_VALVE_TRANSFER_BUSY);
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_SERVICE, false, false, false, false) ==
           HWC::MANUAL_VALVE_RECOVERY_BLOCKED);
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_SERVICE, false, false, false, true) ==
           HWC::MANUAL_VALVE_OK);
    assert(control.active());
    assert(control.operatingMode() == HWC::MANUAL_VALVE_OPERATING_SERVICE);
    assert(!control.physicalLevelCheckConfirmed());
    assert(control.command() == HWC::MANUAL_VALVES_CLOSE_ALL);
    assert(control.command(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER, false) == HWC::MANUAL_VALVE_SAFETY_BLOCKED);
    assert(control.command(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER, true) == HWC::MANUAL_VALVE_OK);
    assert(control.command(HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER, true) == HWC::MANUAL_VALVE_CLOSE_BEFORE_SWITCH);
    assert(control.command(HWC::MANUAL_VALVES_CLOSE_ALL, false) == HWC::MANUAL_VALVE_OK);
    control.deactivate();
    assert(!control.active());
    assert(control.operatingMode() == HWC::MANUAL_VALVE_OPERATING_NONE);

    // Emergency mode is unavailable when all relevant sensors and peer data are healthy.
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_EMERGENCY, true, false, false, true) ==
           HWC::MANUAL_VALVE_SENSOR_FAULT_REQUIRED);
    assert(!control.active());
    // With an actual relevant sensor/peer fault, explicit physical confirmation is required.
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_EMERGENCY, false, true, false, true) ==
           HWC::MANUAL_VALVE_CONFIRMATION_REQUIRED);
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_EMERGENCY, true, true, false, true) ==
           HWC::MANUAL_VALVE_OK);
    assert(control.operatingMode() == HWC::MANUAL_VALVE_OPERATING_EMERGENCY);
    assert(control.physicalLevelCheckConfirmed());
    assert(control.command(HWC::MANUAL_VALVE_OPEN_WELL_TO_LOWER, true) == HWC::MANUAL_VALVE_OK);
    control.stop();
    assert(control.command() == HWC::MANUAL_VALVES_CLOSE_ALL);
    control.deactivate();
    assert(!control.active());
    assert(!control.physicalLevelCheckConfirmed());

    // Invalid mode never opens a session.
    assert(control.activate(HWC::MANUAL_VALVE_OPERATING_NONE, false, false, false, true) ==
           HWC::MANUAL_VALVE_MODE_INVALID);

    // A newly constructed controller models a reboot: no mode or open request survives.
    HWC::ManualValveControl afterReboot;
    assert(!afterReboot.active());
    assert(afterReboot.command() == HWC::MANUAL_VALVES_CLOSE_ALL);
    assert(afterReboot.operatingMode() == HWC::MANUAL_VALVE_OPERATING_NONE);

    puts("PASS: service/emergency manual valve control state machine");
    return 0;
}
