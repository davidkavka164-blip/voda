#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
FILE=firmware/ESP32_2/ESP32_2.ino
if grep -Eq "localWebServer[.]on\(\"/volume-|upperVolumeStore|estimateUpperTankVolume|name='(reserve|valveOpen)'" "$FILE"; then
  echo 'FAIL: the upper pump dashboard still includes historical volume or household controls' >&2
  exit 1
fi
grep -Fq 'const char* names[] = {"depth", "trip", "recovery"};' "$FILE"
grep -Fq 'HWC::isUpperTankLimitsValid(candidate)' "$FILE"
grep -Fq 'upperLimitsStore.save(candidate)' "$FILE"
grep -Fq 'localWebServer.on("/sensor-recovery", HTTP_POST, handleLocalSensorRecovery);' "$FILE"
grep -Fq 'localWebServer.on("/pump-emergency", HTTP_POST, handleLocalPumpEmergency);' "$FILE"
POWER_CUT_TEST=tests/test_nvs_upper_tank_volume_calibration_powercut.cpp
[ -f "$POWER_CUT_TEST" ] || { echo 'FAIL: missing upper-volume NVS power-cut model test' >&2; exit 1; }
for token in 'WRITE_INTERRUPTED_BEFORE_COMMIT' 'WRITE_COMMITTED_BUT_REPORT_FAILURE' 'REMOVE_INTERRUPTED_BEFORE_COMMIT' 'REMOVE_COMMITTED_BUT_REPORT_FAILURE' 'READ_FAILURE' 'afterInterruptedCompletion' 'afterCommittedButUnreported'; do
  grep -Fq "$token" "$POWER_CUT_TEST" || { echo "FAIL: missing upper-volume power-cut scenario: $token" >&2; exit 1; }
done
grep -Fq 'test_nvs_upper_tank_volume_calibration_powercut' tests/run_native_tests.sh
grep -Fq 'test_nvs_upper_tank_volume_calibration_powercut' tests/.run_native_tests_asan.sh
echo 'PASS: ESP32 #2 exposes local pump limits only; historical volume storage remains model-tested'
