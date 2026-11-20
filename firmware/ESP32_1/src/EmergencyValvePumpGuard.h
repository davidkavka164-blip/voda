#ifndef HWC_EMERGENCY_VALVE_PUMP_GUARD_H
#define HWC_EMERGENCY_VALVE_PUMP_GUARD_H

namespace HWC {

// Emergency valve operation may bypass a failed/missing source-level sensor.
// When the well sensor is valid, however, the existing well-pump dry-run permit
// remains authoritative and must not be bypassed by opening the well route.
inline bool emergencyWellValvePumpGuardAllows(bool wellSensorUsable,
                                               bool wellPumpPermit) {
    return !wellSensorUsable || wellPumpPermit;
}

} // namespace HWC

#endif
