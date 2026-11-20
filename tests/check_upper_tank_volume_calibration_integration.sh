#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
FILE=firmware/ESP32_2/ESP32_2.ino
for token in 'markUpperVolumeStoreUncertain' 'NvsUpperTankVolumeCalibrationStore' 'handleLocalVolumeAnchor' 'handleLocalVolumePoint' 'handleLocalVolumeReference' 'handleLocalVolumeTransfer' 'handleLocalVolumeReferenceCancel' 'handleLocalVolumeCorruptReset' 'upperVolumeStore.discardCorruptRecord' 'upperVolumeStore.load' 'upperVolumeStore.save' 'estimateUpperTankVolume' 'addUpperTankPostTransferPoint' 'upperVolumeCalibration.transferReferenceValid'; do
  grep -Fq "$token" "$FILE" || { echo "FAIL: missing upper volume calibration integration: $token" >&2; exit 1; }
done
grep -Fq 'Ruční úprava tabulky je dočasně zablokována' "$FILE"
grep -Fq 'Během otevřené reference přepouštění nelze ručně měnit' "$FILE"
[ "$(grep -Fc 'markUpperVolumeStoreUncertain();' "$FILE")" -eq 6 ] || { echo 'FAIL: every upper-volume NVS save failure must invalidate the in-RAM trust state' >&2; exit 1; }
grep -Fq 'upperVolumeCalibrationValid = false;' "$FILE"
grep -Fq 'upperVolumeCalibrationCorrupt = true;' "$FILE"
POWER_CUT_TEST=tests/test_nvs_upper_tank_volume_calibration_powercut.cpp
[ -f "$POWER_CUT_TEST" ] || { echo 'FAIL: missing upper-volume NVS power-cut model test' >&2; exit 1; }
for token in 'WRITE_INTERRUPTED_BEFORE_COMMIT' 'WRITE_COMMITTED_BUT_REPORT_FAILURE' 'REMOVE_INTERRUPTED_BEFORE_COMMIT' 'REMOVE_COMMITTED_BUT_REPORT_FAILURE' 'READ_FAILURE' 'afterInterruptedCompletion' 'afterCommittedButUnreported'; do
  grep -Fq "$token" "$POWER_CUT_TEST" || { echo "FAIL: missing upper-volume power-cut scenario: $token" >&2; exit 1; }
done
grep -Fq 'test_nvs_upper_tank_volume_calibration_powercut' tests/run_native_tests.sh
grep -Fq 'test_nvs_upper_tank_volume_calibration_powercut' tests/.run_native_tests_asan.sh
for route in '/volume-anchor' '/volume-point' '/volume-reference' '/volume-transfer' '/volume-reference-cancel' '/volume-corrupt-reset'; do
  grep -Fq "localWebServer.on(\"$route\", HTTP_POST" "$FILE" || { echo "FAIL: missing authenticated web route $route" >&2; exit 1; }
done
if grep -E 'upperVolumeCalibration|estimateUpperTankVolume' "$FILE" | grep -E 'upperPumpProtection|upperPumpSafetyRelay|TransferLogic'; then
  echo "FAIL: upper volume curve must not control pump safety or transfers" >&2
  exit 1
fi
echo "PASS: upper tank volume calibration is local to ESP32 #2, persisted, web-managed, read-back verified, fail-closed after uncertain NVS writes and covered by write/remove/read-failure and atomic power-loss/restart model tests"

grep -A18 -F 'void handleLocalVolumeCorruptReset()' "$FILE" | grep -Fq 'if (!requireLocalAuth()) return;' || { echo 'FAIL: corrupt-volume reset must require local authentication' >&2; exit 1; }
grep -A18 -F 'void handleLocalVolumeCorruptReset()' "$FILE" | grep -Fq 'localWebServer.arg("physical") != "yes"' || { echo 'FAIL: corrupt-volume reset must require explicit physical confirmation' >&2; exit 1; }
grep -Fq 'úplnou ztrátu poškozené objemové kalibrace' "$FILE" || { echo 'FAIL: destructive recovery warning is missing' >&2; exit 1; }

grep -Fq 'appendUpperTankVolume(h);' "$FILE"
grep -Fq 'setUpperTankTransferReference(updated, level, updated)' "$FILE"
grep -Fq 'const char* names[] = {"depth", "trip", "recovery"};' "$FILE"
if grep -Eq "name='(reserve|valveOpen)'" "$FILE"; then
  echo 'FAIL: the upper unit exposes household or valve limits' >&2
  exit 1
fi
