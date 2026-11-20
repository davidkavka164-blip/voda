#include "PumpSafetyRelayOutput.h"

namespace HWC {
PumpSafetyRelayOutput::PumpSafetyRelayOutput()
    : pin_(0), activeHigh_(true), enabled_(false), ready_(false),
      energized_(false), writer_(0) {}

bool PumpSafetyRelayOutput::levelFor(bool energized) const {
    return energized ? activeHigh_ : !activeHigh_;
}

bool PumpSafetyRelayOutput::begin(uint8_t pin, bool activeHigh, bool commissioningEnabled,
                                  PumpSafetyRelayWriteFn writer) {
    if (ready_ && writer_ != 0 && energized_) writer_(pin_, levelFor(false));
    ready_ = false;
    energized_ = false;
    pin_ = pin;
    activeHigh_ = activeHigh;
    enabled_ = commissioningEnabled;
    writer_ = writer;
    if (pin_ == 0 || writer_ == 0) {
        writer_ = 0;
        enabled_ = false;
        return false;
    }
    // Always command the de-energized state before reporting ready.
    writer_(pin_, levelFor(false));
    ready_ = true;
    return true;
}

bool PumpSafetyRelayOutput::applyPermit(bool permit) {
    if (!ready_ || writer_ == 0) return false;
    const bool nextEnergized = enabled_ && permit;
    if (nextEnergized != energized_) {
        writer_(pin_, levelFor(nextEnergized));
        energized_ = nextEnergized;
    }
    return true;
}

bool PumpSafetyRelayOutput::forceOff() {
    if (!ready_ || writer_ == 0) return false;
    if (energized_) writer_(pin_, levelFor(false));
    energized_ = false;
    return true;
}

bool PumpSafetyRelayOutput::ready() const { return ready_; }
bool PumpSafetyRelayOutput::energized() const { return energized_; }
bool PumpSafetyRelayOutput::commissioningEnabled() const { return enabled_; }
} // namespace HWC
