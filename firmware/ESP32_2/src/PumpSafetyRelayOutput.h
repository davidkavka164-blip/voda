#ifndef HWC_PUMP_SAFETY_RELAY_OUTPUT_H
#define HWC_PUMP_SAFETY_RELAY_OUTPUT_H

#include <stdint.h>

namespace HWC {

typedef void (*PumpSafetyRelayWriteFn)(uint8_t pin, bool high);

// Drives one independent motor-supply safety relay. A permit energizes the relay
// only when commissioning explicitly enables the output. Otherwise it remains OFF.
class PumpSafetyRelayOutput {
public:
    PumpSafetyRelayOutput();
    bool begin(uint8_t pin, bool activeHigh, bool commissioningEnabled,
               PumpSafetyRelayWriteFn writer);
    bool applyPermit(bool permit);
    bool forceOff();
    bool ready() const;
    bool energized() const;
    bool commissioningEnabled() const;
private:
    uint8_t pin_;
    bool activeHigh_;
    bool enabled_;
    bool ready_;
    bool energized_;
    PumpSafetyRelayWriteFn writer_;
    bool levelFor(bool energized) const;
};

} // namespace HWC

#endif
