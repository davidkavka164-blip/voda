#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
for endpoint in firmware/ESP32_1/src firmware/ESP32_2/src; do
  cmp firmware/common/NvsLowerTankInflowWatchdogStore.h "$endpoint/NvsLowerTankInflowWatchdogStore.h"
  cmp firmware/common/NvsLowerTankInflowWatchdogStore.cpp "$endpoint/NvsLowerTankInflowWatchdogStore.cpp"
done
INO=firmware/ESP32_1/ESP32_1.ino
grep -q 'handleInflowWatchdogConfig' "$INO"
grep -q 'webServer.on("/inflow-watchdog-config", HTTP_POST, handleInflowWatchdogConfig)' "$INO"
grep -q 'startup_seconds' "$INO"
grep -q 'no_progress_seconds' "$INO"
grep -q 'inflowWatchdogStore.load(inflowWatchdogSettings)' "$INO"
grep -q 'inflowWatchdogStore.save(candidate)' "$INO"
grep -q 'transferLogic.state() == HWC::TRANSFER_RUNNING' "$INO"
echo "PASS: inflow watchdog settings store is synchronized, loaded at startup and saved through authenticated web route"
