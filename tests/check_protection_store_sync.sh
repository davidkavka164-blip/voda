#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for module in NvsWellProtectionStore NvsUpperTankProtectionStore NvsLowerTankPumpProtectionStore; do
  cmp "firmware/common/$module.h" "firmware/ESP32_1/src/$module.h"
  cmp "firmware/common/$module.h" "firmware/ESP32_2/src/$module.h"
  cmp "firmware/common/$module.cpp" "firmware/ESP32_1/src/$module.cpp"
  cmp "firmware/common/$module.cpp" "firmware/ESP32_2/src/$module.cpp"
done
printf 'PASS: protection-store headers and implementations are byte-identical across common and both ESP32 sketches\n'
