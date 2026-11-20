#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
SKETCH="firmware/ESP32_1/ESP32_1.ino"
! grep -q 'peerStatusMonitor.consumeBootEpochChanged()' "$SKETCH"
! grep -q 'transferLogic.forceAbort(HWC::TRANSFER_REASON_COMMUNICATION)' "$SKETCH"
echo 'PASS: upper transfer no longer depends on peer boot epochs or inter-unit communication'
