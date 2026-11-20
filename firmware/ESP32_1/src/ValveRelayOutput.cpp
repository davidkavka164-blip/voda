#include "ValveRelayOutput.h"

namespace HWC {

ValveRelayOutput::ValveRelayOutput()
    : wellRelayPin_(0), upperRelayPin_(0), activeHigh_(true), writer_(0),
      ready_(false), wellEnergized_(false), upperEnergized_(false),
      appliedCommand_(MANUAL_VALVES_CLOSE_ALL) {}

bool ValveRelayOutput::begin(uint8_t wellRelayPin, uint8_t upperRelayPin,
                             bool activeHigh, WriteLevelFn writer) {
    ready_ = false;
    wellEnergized_ = false;
    upperEnergized_ = false;
    appliedCommand_ = MANUAL_VALVES_CLOSE_ALL;
    if (!writer || wellRelayPin == upperRelayPin) return false;

    wellRelayPin_ = wellRelayPin;
    upperRelayPin_ = upperRelayPin;
    activeHigh_ = activeHigh;
    writer_ = writer;
    // Explicitly command both relays OFF before the driver becomes ready.
    writeRelay(wellRelayPin_, false);
    writeRelay(upperRelayPin_, false);
    ready_ = true;
    return true;
}

void ValveRelayOutput::writeRelay(uint8_t pin, bool energized) {
    if (writer_) writer_(pin, energized ? activeHigh_ : !activeHigh_);
}

bool ValveRelayOutput::apply(ManualValveCommand command) {
    if (!ready_) return false;
    if (command != MANUAL_VALVES_CLOSE_ALL &&
        command != MANUAL_VALVE_OPEN_WELL_TO_LOWER &&
        command != MANUAL_VALVE_OPEN_UPPER_TO_LOWER) {
        return forceClose();
    }
    if (command == appliedCommand_) return true;

    // Break-before-make: both route relays are switched OFF before one is ON.
    writeRelay(wellRelayPin_, false);
    writeRelay(upperRelayPin_, false);
    wellEnergized_ = false;
    upperEnergized_ = false;
    appliedCommand_ = MANUAL_VALVES_CLOSE_ALL;

    if (command == MANUAL_VALVE_OPEN_WELL_TO_LOWER) {
        writeRelay(wellRelayPin_, true);
        wellEnergized_ = true;
    } else if (command == MANUAL_VALVE_OPEN_UPPER_TO_LOWER) {
        writeRelay(upperRelayPin_, true);
        upperEnergized_ = true;
    }
    appliedCommand_ = command;
    return true;
}

bool ValveRelayOutput::forceClose() {
    if (!ready_) return false;
    if (!wellEnergized_ && !upperEnergized_ &&
        appliedCommand_ == MANUAL_VALVES_CLOSE_ALL) return true;
    writeRelay(wellRelayPin_, false);
    writeRelay(upperRelayPin_, false);
    wellEnergized_ = false;
    upperEnergized_ = false;
    appliedCommand_ = MANUAL_VALVES_CLOSE_ALL;
    return true;
}

bool ValveRelayOutput::ready() const { return ready_; }
ManualValveCommand ValveRelayOutput::appliedCommand() const { return appliedCommand_; }
bool ValveRelayOutput::wellRelayEnergized() const { return wellEnergized_; }
bool ValveRelayOutput::upperRelayEnergized() const { return upperEnergized_; }

} // namespace HWC
