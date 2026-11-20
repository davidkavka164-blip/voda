#!/bin/sh
set -eu
SKETCH="tools/W5500_DUAL_INTERFACE_DIAGNOSTIC/W5500_DUAL_INTERFACE_DIAGNOSTIC.ino"
PROD="firmware/ESP32_2/ESP32_2.ino"
test -f "$SKETCH"
grep -q '#include <Ethernet.h>' "$SKETCH"
grep -q '#include <WebServer.h>' "$SKETCH"
grep -q 'EthernetServer ethernetServer(80)' "$SKETCH"
grep -q 'WebServer apServer(80)' "$SKETCH"
grep -q 'apServer.handleClient()' "$SKETCH"
grep -q 'processEthernetClient()' "$SKETCH"
grep -q 'at most 64 received bytes per loop pass' "$SKETCH"
grep -q 'char ethernetRequestLine\[96\]' "$SKETCH"
grep -q 'char ethernetHeaderLine\[128\]' "$SKETCH"
grep -q 'ethernetHeaderCount >= 16U' "$SKETCH"
! grep -q 'readStringUntil' "$SKETCH"
! grep -q 'client.setTimeout' "$SKETCH"
grep -q 'PIN_W5500_CS   = 27' "$SKETCH"
grep -q 'PIN_W5500_RST  = 26' "$SKETCH"
grep -q 'AP_PASSWORD' "$SKETCH"
# The bench sketch must not be wired into production and must not contain control/OTA routes.
! grep -q 'W5500_DUAL_INTERFACE_DIAGNOSTIC' "$PROD"
! grep -Eqi 'pump|valve|Update\.begin|/update|Preferences' "$SKETCH"
printf '%s\n' 'PASS: isolated W5500 + service AP diagnostic sketch static checks'
