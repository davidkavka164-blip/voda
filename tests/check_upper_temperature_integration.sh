#!/bin/sh
set -eu
SKETCH=firmware/ESP32_2/ESP32_2.ino
PINS=firmware/ESP32_2/src/HardwarePins.h
COMMON=firmware/common/WaterTemperature.h
[ -f "$COMMON" ]
grep -q '#include <OneWire.h>' "$SKETCH"
grep -q '#include <DallasTemperature.h>' "$SKETCH"
grep -q 'WATER_TEMPERATURE_DATA = 4' "$PINS"
grep -q 'maintainWaterTemperature(now)' "$SKETCH"
grep -q 'WATER_TEMPERATURE_MAX_AGE_MS' "$SKETCH"
grep -q 'DS18B20 data GPIO=%u (informational measurement)' "$SKETCH"
if grep -q 'temperature sensor planned GPIO=.*not read yet' "$SKETCH"; then
  echo 'FAIL: startup diagnostics still describe implemented DS18B20 as not read yet'
  exit 1
fi
grep -q 'Teplota je pouze informativní' "$SKETCH"
grep -q 'isWaterTemperatureFresh' "$SKETCH"
if grep -q 'temperature.*pumpPermit\|pumpPermit.*temperature' "$SKETCH"; then
  echo 'FAIL: temperature appears coupled to pump permit'
  exit 1
fi
printf '%s\n' 'PASS: upper DS18B20 integration, freshness gate and informational-only separation'
