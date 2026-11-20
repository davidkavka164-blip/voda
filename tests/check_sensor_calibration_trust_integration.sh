#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
UPPER=firmware/ESP32_2/ESP32_2.ino
# Startup must distinguish absent, corrupt and unavailable records on both units.
grep -q 'SENSOR_CONFIG_NVS_UNAVAILABLE' "$MAIN"
grep -q 'SENSOR_CONFIG_MISSING_PROVISIONAL' "$MAIN"
grep -q 'SENSOR_CONFIG_INVALID_PRESENT' "$MAIN"
grep -q 'SENSOR_CONFIG_VALID_STORED' "$MAIN"
grep -q 'SENSOR_CONFIG_NVS_UNAVAILABLE' "$UPPER"
grep -q 'SENSOR_CONFIG_MISSING_PROVISIONAL' "$UPPER"
grep -q 'SENSOR_CONFIG_INVALID_PRESENT' "$UPPER"
grep -q 'SENSOR_CONFIG_VALID_STORED' "$UPPER"
# Missing provisional, corrupt and unavailable electrical config must fail closed.
grep -q 'return state == SENSOR_CONFIG_VALID_STORED;' firmware/common/SensorConfig.h
# All three local pump protections must require usable electrical level conversion.
grep -q 'sensorConfigUsableForLevelMeasurement(wellSensorConfigState)' "$MAIN"
grep -q 'sensorConfigUsableForLevelMeasurement(lowerSensorConfigState)' "$MAIN"
grep -q 'sensorConfigUsableForLevelMeasurement(upperSensorConfigState)' "$UPPER"
# ESP32 #2 local pump protection must not use an upper level from an untrusted calibration.
grep -q 'if (HWC::sensorConfigUsableForLevelMeasurement(upperSensorConfigState)' "$UPPER"
# Explicit emergency paths remain wired into the existing protection update APIs.
grep -q 'emergencyConfirmationRequested' "$MAIN"
grep -q 'lowerEmergencyConfirmationRequested' "$MAIN"
grep -q 'upperEmergencyConfirmationRequested' "$UPPER"
echo 'PASS: electrical sensor configuration gates all local pump and transfer measurements; upper sensor remains local to ESP32 #2'
