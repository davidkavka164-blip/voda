#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for f in firmware/common/WaterMath.h firmware/ESP32_1/src/WaterMath.h firmware/ESP32_2/src/WaterMath.h; do
  grep -q 'Informational tank-use reference only; never a pump/transfer control threshold' "$f"
done
# The 11 cm constant may only be consumed by the ESP32 #1 informational display.
COUNT=$(grep -Rho 'LOWER_TANK_MIN_WORKING_LEVEL_CM' firmware --include='*.cpp' --include='*.h' --include='*.ino' | wc -l | tr -d ' ')
[ "$COUNT" = "4" ] || { echo "FAIL: unexpected 11 cm constant references ($COUNT)" >&2; exit 1; }
if grep -q 'LOWER_TANK_MIN_WORKING_LEVEL_CM' firmware/common/LowerTankPumpProtection.cpp firmware/ESP32_1/src/LowerTankPumpProtection.cpp; then
  echo 'FAIL: 11 cm value is used by lower pump protection logic' >&2; exit 1
fi
grep -q 'Pracovní minimum 11 cm je pouze informativní' firmware/ESP32_1/ESP32_1.ino
grep -q 'výhradně nastavená minimální hladina a hystereze' firmware/ESP32_1/ESP32_1.ino
echo 'PASS: 11 cm remains informational only; lower pump protection uses configured minimum and hysteresis'
