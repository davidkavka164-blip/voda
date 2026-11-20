#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
SKETCH="firmware/ESP32_2/ESP32_2.ino"
! grep -Eq 'peerBootId|peerSequence|initializePeerBootIdentity' "$SKETCH"
grep -Fq 'ethernetStatus.begin(localAdminPassword);' "$SKETCH"
echo "PASS: no inter-unit boot identity; ESP32 #2 starts its independent Ethernet interface"
