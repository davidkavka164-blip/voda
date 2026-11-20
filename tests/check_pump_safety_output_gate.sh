#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
MAIN1="firmware/ESP32_1/ESP32_1.ino"
MAIN2="firmware/ESP32_2/ESP32_2.ino"
for f in firmware/common/HardwarePins.h firmware/ESP32_1/src/HardwarePins.h firmware/ESP32_2/src/HardwarePins.h; do
  grep -q 'PUMP_SAFETY_OUTPUTS_ENABLED = true' "$f" || {
    echo "FAIL: pump safety outputs must be enabled from the proven standalone-well wiring in $f" >&2; exit 1;
  }
done
for f in "$MAIN1" "$MAIN2"; do
  grep -q 'PumpSafetyRelayOutput' "$f" || { echo "FAIL: missing pump relay output driver in $f" >&2; exit 1; }
  grep -q 'PUMP_SAFETY_OUTPUTS_ENABLED' "$f" || { echo "FAIL: missing enabled relay output in $f" >&2; exit 1; }
  grep -q 'sync.*PumpSafetyRelay' "$f" || { echo "FAIL: missing runtime pump relay synchronization in $f" >&2; exit 1; }
done
for token in 'WELL_PUMP_SAFETY_RELAY = 25' 'LOWER_TANK_PUMP_SAFETY_RELAY = 32'; do
  grep -q "$token" firmware/ESP32_1/src/HardwarePins.h
done
grep -q 'UPPER_TANK_PUMP_SAFETY_RELAY = 25' firmware/ESP32_2/src/HardwarePins.h
grep -q 'sampleIsFresh(haveWellReading' "$MAIN1"
grep -q 'sampleIsFresh(haveLowerTankReading' "$MAIN1"
grep -q 'nowMs - lastReportMs) <= 5000U' "$MAIN2"
grep -Fq 'pumpProtectionState.emergencyModeActive ||' "$MAIN1"
grep -Fq 'lowerPumpState.emergencyModeActive ||' "$MAIN1"
grep -Fq 'upperPumpProtectionState.emergencyModeActive ||' "$MAIN2"
# Every pump relay is commanded OFF at the beginning of setup, before the startup delay/network work.
sed -n '/void setup()/,/delay(250)/p' "$MAIN1" | grep -Fq 'digitalWrite(HwcPins::Esp32_1::WELL_PUMP_SAFETY_RELAY, LOW)'
sed -n '/void setup()/,/delay(250)/p' "$MAIN1" | grep -Fq 'digitalWrite(HwcPins::Esp32_1::LOWER_TANK_PUMP_SAFETY_RELAY, LOW)'
sed -n '/void setup()/,/delay(250)/p' "$MAIN2" | grep -Fq 'digitalWrite(HwcPins::Esp32_2::UPPER_TANK_PUMP_SAFETY_RELAY, LOW)'
# Emergency modes are deliberately not persisted.
grep -Fq 'emergencyMode_(false)' firmware/common/WellPumpProtection.cpp
grep -Fq 'emergencyMode_(false)' firmware/common/UpperTankProtection.cpp
grep -Fq 'emergencyMode_(false)' firmware/common/LowerTankPumpProtection.cpp
echo 'PASS: all three pump safety outputs are enabled; normal mode is freshness-gated; confirmed emergency mode bypasses sensor freshness; setup forces OFF before startup delay; emergency modes reset after reboot'
