#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
for token in "/manual-valve-mode" "/manual-valve-command" "manualValveControl.activate" "MANUAL_VALVE_OPERATING_SERVICE" "MANUAL_VALVE_OPERATING_EMERGENCY" "mode' value='service'" "mode' value='emergency'" "manualValveControl.command" "manualValveControl.stop" "manualValveControl.deactivate" "manualValveControl.active()" "Automatický transfer je blokován" "ValveRelayOutput" "syncManualValveRelayOutput" "manualValveRelayOutput.apply" "manualValveRelayOutput.forceClose"; do
  grep -Fq "$token" "$MAIN" || { echo "FAIL: missing manual valve integration token: $token" >&2; exit 1; }
done
for token in "EmergencyValvePumpGuard.h" "emergencyWellValvePumpGuardAllows" "wellSensorUsable" "pumpProtectionState.pumpPermit" "Ochrana horního čerpadla běží místně na ESP32 #2"; do
  grep -Fq "$token" "$MAIN" "$ROOT/firmware/common/EmergencyValvePumpGuard.h" || { echo "FAIL: missing emergency pump-protection preservation: $token" >&2; exit 1; }
done
for token in "MANUAL_VALVE_SENSOR_FAULT_BLOCKS_SERVICE" "MANUAL_VALVE_SENSOR_FAULT_REQUIRED" "MANUAL_VALVE_OPERATING_EMERGENCY" "physicalLevelCheckConfirmed()" "wellFresh && lastWellReading.result.calibratedLevelCm <= wellProtectionConfig.transferMinimumLevelCm" "Emergency mode is an intentional operator override" "emergencyWellValvePumpGuardAllows"; do
  grep -Fq "$token" "$MAIN" "$ROOT/firmware/common/ManualValveControl.cpp" || { echo "FAIL: missing manual valve safety gate: $token" >&2; exit 1; }
done
for token in "VALVE_1_RELAY, LOW" "VALVE_2_RELAY, LOW" "pinMode(HwcPins::Esp32_1::VALVE_1_RELAY, OUTPUT)" "pinMode(HwcPins::Esp32_1::VALVE_2_RELAY, OUTPUT)"; do
  grep -Fq "$token" "$MAIN" || { echo "FAIL: missing fail-safe relay initialization: $token" >&2; exit 1; }
done
grep -Fq 'PUMP_SAFETY_OUTPUTS_ENABLED = true' firmware/ESP32_1/src/HardwarePins.h
grep -Fq 'PUMP_SAFETY_OUTPUTS_ENABLED = true' firmware/ESP32_2/src/HardwarePins.h
grep -Fq 'syncPumpSafetyRelayOutputs(millis())' "$MAIN"
grep -Fq 'syncUpperPumpSafetyRelay(millis())' firmware/ESP32_2/ESP32_2.ino
if grep -Eq "digitalWrite\([^,]*(VALVE_1_RELAY|VALVE_2_RELAY)" firmware/ESP32_2/ESP32_2.ino; then
  echo "FAIL: ESP32 #2 must not drive lower-tank valve relays" >&2
  exit 1
fi
[ -f firmware/ESP32_1/src/ValveRelayOutput.cpp ] || { echo "FAIL: relay output driver missing" >&2; exit 1; }
echo "PASS: ESP32 #1 manual valve relay output is integrated; pump relay outputs use the proven HIGH-mode wiring and ESP32 #2 has no valve outputs"
