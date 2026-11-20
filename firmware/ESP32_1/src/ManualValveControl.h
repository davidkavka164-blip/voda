#ifndef HWC_MANUAL_VALVE_CONTROL_H
#define HWC_MANUAL_VALVE_CONTROL_H

#include <stdint.h>

namespace HWC {

enum ManualValveModeState : uint8_t {
    MANUAL_VALVE_MODE_INACTIVE = 0,
    MANUAL_VALVE_MODE_ACTIVE = 1
};

enum ManualValveOperatingMode : uint8_t {
    MANUAL_VALVE_OPERATING_NONE = 0,
    MANUAL_VALVE_OPERATING_SERVICE = 1,
    MANUAL_VALVE_OPERATING_EMERGENCY = 2
};

enum ManualValveCommand : uint8_t {
    MANUAL_VALVES_CLOSE_ALL = 0,
    MANUAL_VALVE_OPEN_WELL_TO_LOWER = 1,
    MANUAL_VALVE_OPEN_UPPER_TO_LOWER = 2
};

enum ManualValveResult : uint8_t {
    MANUAL_VALVE_OK = 0,
    MANUAL_VALVE_CONFIRMATION_REQUIRED,
    MANUAL_VALVE_SENSOR_FAULT_REQUIRED,
    MANUAL_VALVE_SENSOR_FAULT_BLOCKS_SERVICE,
    MANUAL_VALVE_TRANSFER_BUSY,
    MANUAL_VALVE_RECOVERY_BLOCKED,
    MANUAL_VALVE_NOT_ACTIVE,
    MANUAL_VALVE_SAFETY_BLOCKED,
    MANUAL_VALVE_CLOSE_BEFORE_SWITCH,
    MANUAL_VALVE_MODE_INVALID
};

// RAM-only manual valve state. Service mode requires healthy sensors. Emergency
// mode is available only when a relevant sensor/peer fault exists and after explicit
// physical level-check confirmation. Its OPEN command bypasses process sensor gates;
// relay readiness, output arbitration and the transfer/restart interlocks remain.
class ManualValveControl {
public:
    ManualValveControl();

    ManualValveResult activate(ManualValveOperatingMode mode,
                               bool physicalLevelCheckConfirmed,
                               bool relevantSensorFaultPresent,
                               bool transferBusy,
                               bool transferRecoveryReady);
    ManualValveResult command(ManualValveCommand command, bool safetyPermitted);
    void stop();
    void deactivate();

    ManualValveModeState state() const;
    ManualValveOperatingMode operatingMode() const;
    ManualValveCommand command() const;
    bool active() const;
    bool physicalLevelCheckConfirmed() const;

private:
    ManualValveModeState state_;
    ManualValveOperatingMode operatingMode_;
    ManualValveCommand command_;
    bool physicalLevelCheckConfirmed_;
};

} // namespace HWC

#endif
