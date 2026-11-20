#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
# The physical relay driver must be part of the automatic transfer path.
grep -Fq 'HWC::ManualValveCommand transferValveCommandForRelay()' "$MAIN"
grep -Fq 'case HWC::VALVE_OPEN_WELL_TO_LOWER:' "$MAIN"
grep -Fq 'case HWC::VALVE_OPEN_UPPER_TO_LOWER:' "$MAIN"
grep -Fq 'const HWC::ManualValveCommand desired = manualValveControl.active()' "$MAIN"
grep -Fq ': transferValveCommandForRelay();' "$MAIN"
grep -Fq 'in.systemReady = manualValveRelayOutputReady' "$MAIN"
grep -Fq 'Relay-board logic/module supply: 5 V' firmware/ESP32_1/src/HardwarePins.h
grep -Fq 'if (!manualValveRelayOutputReady)' "$MAIN"
grep -Fq 'transferLogic.forceAbort(HWC::TRANSFER_REASON_SYSTEM_NOT_READY)' "$MAIN"
grep -Fq 'transferRestartGuard.markActive(in.lowerLevelCm, transferOutputs.targetLowerLevelCm)' "$MAIN"
grep -Fq 'updateTransferControl(millis());' "$MAIN"
grep -Fq 'syncManualValveRelayOutput();' "$MAIN"
# Manual mode has priority. Pump outputs are enabled from the already working standalone-well relay configuration.
grep -Fq 'PUMP_SAFETY_OUTPUTS_ENABLED = true' firmware/ESP32_1/src/HardwarePins.h
grep -Fq 'PUMP_SAFETY_OUTPUTS_ENABLED = true' firmware/ESP32_2/src/HardwarePins.h
grep -Fq 'syncPumpSafetyRelayOutputs(millis())' "$MAIN"
grep -Fq 'syncUpperPumpSafetyRelay(millis())' firmware/ESP32_2/ESP32_2.ino
if grep -Eq 'digitalWrite\([^,]*(VALVE_1_RELAY|VALVE_2_RELAY)' firmware/ESP32_2/ESP32_2.ino; then
  echo 'FAIL: ESP32 #2 must not drive lower-tank valve relays' >&2; exit 1
fi
# The source must no longer advertise the automatic path as simulation-only.
if grep -Eq 'SIMULACE ONLY|simulation-only|Automatický transfer je stále pouze simulace' "$MAIN"; then
  echo 'FAIL: stale simulation-only wording remains in ESP32 #1 UI/source' >&2; exit 1
fi
echo 'PASS: automatic transfer drives the shared valve relay driver; journal/readiness gates remain; pump safety outputs use the proven HIGH-mode relay configuration'
