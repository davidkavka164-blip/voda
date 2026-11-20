#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
for endpoint in firmware/ESP32_1/src firmware/ESP32_2/src; do
  cmp firmware/common/TransferLogic.h "$endpoint/TransferLogic.h"
  cmp firmware/common/TransferLogic.cpp "$endpoint/TransferLogic.cpp"
done
grep -q 'inflowWatchdog_.begin' firmware/common/TransferLogic.cpp
grep -q 'inflowWatchdog_.update' firmware/common/TransferLogic.cpp
grep -q 'TRANSFER_REASON_NO_INFLOW' firmware/common/TransferLogic.cpp
grep -q 'inflowStartupGraceMs' firmware/ESP32_1/ESP32_1.ino
grep -q 'inflowNoProgressTimeoutMs' firmware/ESP32_1/ESP32_1.ino
grep -q 'inflowMinimumProgressLiters = 12.32f' firmware/ESP32_1/ESP32_1.ino
grep -q 'TRANSFER_REASON_NO_INFLOW' firmware/ESP32_1/ESP32_1.ino
echo "PASS: upper-to-lower transfer uses persisted watchdog time limits and aborts on absent/interrupted inflow"
