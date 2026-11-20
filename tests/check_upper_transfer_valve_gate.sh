#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
! grep -q 'peerDataValid' firmware/common/TransferLogic.h
! grep -q 'upperTransferValvePermitted' firmware/common/TransferLogic.h
grep -q 'const bool upperSource = inputs.request.source == TRANSFER_SOURCE_UPPER_TANK' firmware/common/TransferLogic.cpp
! grep -q 'inputs.request.source == TRANSFER_SOURCE_UPPER_TANK && !inputs.upperTransferValvePermitted' firmware/common/TransferLogic.cpp
grep -q 'in.systemReady = manualValveRelayOutputReady && in.lowerLevelValid' firmware/ESP32_1/ESP32_1.ino
grep -q 'return lowerFresh;' firmware/ESP32_1/ESP32_1.ino
sh "tests/check_common_module_sync.sh" >/dev/null
echo 'PASS: TransferLogic contract has no peer fields; upper transfer uses local receiving-tank gates'
