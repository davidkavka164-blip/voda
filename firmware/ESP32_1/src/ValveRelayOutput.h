#ifndef HWC_VALVE_RELAY_OUTPUT_H
#define HWC_VALVE_RELAY_OUTPUT_H

#include <stdint.h>
#include "ManualValveControl.h"

namespace HWC {

// Hardware-independent relay mapper for the two permitted valve paths.
// The caller must configure GPIO as outputs and provide a digital-level writer.
// The logical command is deliberately not a claim about physical valve position.
class ValveRelayOutput {
public:
    typedef void (*WriteLevelFn)(uint8_t pin, bool high);

    ValveRelayOutput();

    bool begin(uint8_t wellRelayPin, uint8_t upperRelayPin,
               bool activeHigh, WriteLevelFn writer);
    bool apply(ManualValveCommand command);
    bool forceClose();

    bool ready() const;
    ManualValveCommand appliedCommand() const;
    bool wellRelayEnergized() const;
    bool upperRelayEnergized() const;

private:
    void writeRelay(uint8_t pin, bool energized);

    uint8_t wellRelayPin_;
    uint8_t upperRelayPin_;
    bool activeHigh_;
    WriteLevelFn writer_;
    bool ready_;
    bool wellEnergized_;
    bool upperEnergized_;
    ManualValveCommand appliedCommand_;
};

} // namespace HWC

#endif
