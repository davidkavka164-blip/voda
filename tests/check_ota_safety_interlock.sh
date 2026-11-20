#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
MAIN1="firmware/ESP32_1/ESP32_1.ino"
MAIN2="firmware/ESP32_2/ESP32_2.ino"
# ESP32 #1 OTA must require a .bin file and reject all local pump/transfer/manual-valve activity.
grep -Fq 'filename.endsWith(".bin")' "$MAIN1"
grep -Fq 'pumpProtectionState.emergencyModeActive || lowerPumpState.emergencyModeActive' "$MAIN1"
grep -Fq 'HWC::TRANSFER_PRECHECK' "$MAIN1"
grep -Fq 'HWC::TRANSFER_RUNNING' "$MAIN1"
grep -Fq 'manualValveControl.active()' "$MAIN1"
# Safety state is rechecked while chunks arrive and immediately before finalization.
test "$(grep -Fc 'otaOperationBlockReason()' "$MAIN1")" -ge 4
# Both units block OTA only while an operator-confirmed pump emergency override is active.
grep -Fq 'upperPumpProtectionState.emergencyModeActive' "$MAIN2"
! grep -Fq 'upperPumpProtectionState.pumpPermit || upperPumpProtectionState.emergencyModeActive' "$MAIN2"
grep -Fq 'Update.abort()' "$MAIN2"
# ESP32 #2 rechecks the emergency override immediately before OTA finalization.
awk '/void handleLocalUpdateUpload\(\)/,/^}/' "$MAIN2" | grep -q 'if (upperPumpProtectionState.emergencyModeActive)'
awk '/void handleLocalUpdateUpload\(\)/,/^}/' "$MAIN2" | grep -q 'OTA #2 aborted before finalization'
echo 'PASS: OTA is .bin-only; both ESP32 units block emergency override; ESP32 #1 also blocks active transfer/manual valves and rechecks during upload'

! grep -Fq 'OTA pro ESP32 #2 zatím není implementována' "$MAIN1"
