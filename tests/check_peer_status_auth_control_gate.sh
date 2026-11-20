#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
UPPER=firmware/ESP32_2/ESP32_2.ino
# No peer status, authentication, or remote-level decision may enter production sketches.
! grep -Eq 'PeerStatus|peerUdp|WiFiUDP|EthernetUDP|PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED' "$MAIN" "$UPPER"
# ESP32 #2 keeps its own local upper-pump protection and safety relay.
grep -Fq 'upperPumpProtection.update(' "$UPPER"
grep -Fq 'upperPumpSafetyRelayOutput.applyPermit(permit)' "$UPPER"
# ESP32 #1 upper-valve command is gated by local lower-tank validity; emergency path remains separate.
grep -Fq 'if (command == HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER)' "$MAIN"
grep -Fq 'return lowerFresh;' "$MAIN"
grep -Fq 'MANUAL_VALVE_OPERATING_EMERGENCY' "$MAIN"
grep -Fq 'return command == HWC::MANUAL_VALVE_OPEN_UPPER_TO_LOWER;' "$MAIN"
echo 'PASS: independent local upper-pump protection and local receiving-tank valve gate; no peer control path'
