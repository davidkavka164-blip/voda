#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for d in firmware/ESP32_1/src firmware/ESP32_2/src; do cmp firmware/common/NvsUpperTankLimitsStore.h "$d/NvsUpperTankLimitsStore.h"; cmp firmware/common/NvsUpperTankLimitsStore.cpp "$d/NvsUpperTankLimitsStore.cpp"; done
printf 'PASS: upper-tank-limits NVS store synchronized across common and both ESP32 sketches\n'
