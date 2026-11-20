#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
A="$ROOT/firmware/ESP32_1/ESP32_1.ino"
B="$ROOT/firmware/ESP32_2/ESP32_2.ino"
STORE="$ROOT/firmware/common/NvsSensorConfigStore.cpp"
for f in "$A" "$B"; do
  grep -q 'handle.*SensorRecovery' "$f"
  grep -q 'sensor-recovery.*HTTP_POST' "$f"
  grep -q 'hasArg("physical")' "$f"
  grep -q 'buildSensorConfigRecoveryCandidate' "$f"
  grep -q 'configStore.recover' "$f"
done
grep -q 'getBytes(key, &existing, sizeof(existing)) != sizeof(existing)' "$STORE"
grep -q 'bool NvsSensorConfigStore::recover' "$STORE"
grep -q 'sensorConfigUsableForLevelMeasurement(upperSensorConfigState)' "$B"
echo 'PASS: sensor recovery integration checks'
