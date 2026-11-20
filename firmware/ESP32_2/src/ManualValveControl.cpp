#include "ManualValveControl.h"

namespace HWC {

ManualValveControl::ManualValveControl()
    : state_(MANUAL_VALVE_MODE_INACTIVE), operatingMode_(MANUAL_VALVE_OPERATING_NONE),
      command_(MANUAL_VALVES_CLOSE_ALL), physicalLevelCheckConfirmed_(false) {}

ManualValveResult ManualValveControl::activate(ManualValveOperatingMode mode,
                                                bool physicalLevelCheckConfirmed,
                                                bool relevantSensorFaultPresent,
                                                bool transferBusy,
                                                bool transferRecoveryReady) {
    if (mode != MANUAL_VALVE_OPERATING_SERVICE && mode != MANUAL_VALVE_OPERATING_EMERGENCY)
        return MANUAL_VALVE_MODE_INVALID;
    if (mode == MANUAL_VALVE_OPERATING_SERVICE && relevantSensorFaultPresent)
        return MANUAL_VALVE_SENSOR_FAULT_BLOCKS_SERVICE;
    if (mode == MANUAL_VALVE_OPERATING_EMERGENCY) {
        // Emergency mode is reserved for an actual relevant sensor/peer fault.
        // With healthy measurements and peer communication, normal automatic
        // transfer or the separate service mode is sufficient.
        if (!relevantSensorFaultPresent) return MANUAL_VALVE_SENSOR_FAULT_REQUIRED;
        // The operator must explicitly confirm a physical inspection before enabling.
        if (!physicalLevelCheckConfirmed) return MANUAL_VALVE_CONFIRMATION_REQUIRED;
    }
    if (transferBusy) return MANUAL_VALVE_TRANSFER_BUSY;
    if (!transferRecoveryReady) return MANUAL_VALVE_RECOVERY_BLOCKED;
    state_ = MANUAL_VALVE_MODE_ACTIVE;
    operatingMode_ = mode;
    physicalLevelCheckConfirmed_ = mode == MANUAL_VALVE_OPERATING_EMERGENCY && physicalLevelCheckConfirmed;
    command_ = MANUAL_VALVES_CLOSE_ALL;
    return MANUAL_VALVE_OK;
}

ManualValveResult ManualValveControl::command(ManualValveCommand requested, bool safetyPermitted) {
    if (state_ != MANUAL_VALVE_MODE_ACTIVE) return MANUAL_VALVE_NOT_ACTIVE;
    if (requested == MANUAL_VALVES_CLOSE_ALL) {
        command_ = MANUAL_VALVES_CLOSE_ALL;
        return MANUAL_VALVE_OK;
    }
    if (requested != MANUAL_VALVE_OPEN_WELL_TO_LOWER &&
        requested != MANUAL_VALVE_OPEN_UPPER_TO_LOWER) return MANUAL_VALVE_SAFETY_BLOCKED;
    if (!safetyPermitted) return MANUAL_VALVE_SAFETY_BLOCKED;
    if (command_ != MANUAL_VALVES_CLOSE_ALL && command_ != requested)
        return MANUAL_VALVE_CLOSE_BEFORE_SWITCH;
    command_ = requested;
    return MANUAL_VALVE_OK;
}

void ManualValveControl::stop() { command_ = MANUAL_VALVES_CLOSE_ALL; }

void ManualValveControl::deactivate() {
    command_ = MANUAL_VALVES_CLOSE_ALL;
    state_ = MANUAL_VALVE_MODE_INACTIVE;
    operatingMode_ = MANUAL_VALVE_OPERATING_NONE;
    physicalLevelCheckConfirmed_ = false;
}

ManualValveModeState ManualValveControl::state() const { return state_; }
ManualValveOperatingMode ManualValveControl::operatingMode() const { return operatingMode_; }
ManualValveCommand ManualValveControl::command() const { return command_; }
bool ManualValveControl::active() const { return state_ == MANUAL_VALVE_MODE_ACTIVE; }
bool ManualValveControl::physicalLevelCheckConfirmed() const { return physicalLevelCheckConfirmed_; }

} // namespace HWC
