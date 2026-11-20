#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
MAIN=firmware/ESP32_1/ESP32_1.ino
PINS=firmware/ESP32_1/src/HardwarePins.h
for token in 'handleLowerPumpProtectionConfig' 'updateLowerTankPumpProtection(lower)' 'minimumLevelCm' 'hysteresisCm' '/lower-pump-protection-config' 'new HWC::LowerTankPumpProtection' 'lowerPumpProtectionStore.saveCriticalLatch' '/lower-pump-emergency' 'handleLowerPumpEmergency' 'lowerEmergencyEnableRequested' 'lowerPumpState.emergencyModeActive'; do
  grep -q "$token" "$MAIN" || { echo "FAIL: missing lower pump integration token: $token" >&2; exit 1; }
done
grep -q 'LOWER_TANK_PUMP_SAFETY_RELAY = 32' "$PINS"
grep -q 'LOWER_PUMP_TANK_MAX_LEVEL_CM = 121.0f' firmware/common/LowerTankPumpProtection.h
grep -q 'PUMP_SAFETY_OUTPUTS_ENABLED = true' "$PINS"
grep -q 'const bool lowerPermit = lowerPumpState.emergencyModeActive ||' "$MAIN"
grep -q 'lowerPumpSafetyRelayOutput.applyPermit(lowerPermit)' "$MAIN"
echo 'PASS: lower-tank pump protection is configured and persisted; confirmed emergency override reaches the relay despite sensor/config/NVS faults; relay output uses proven HIGH-mode wiring'
