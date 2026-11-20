#!/bin/sh
set -eu
SKETCH=firmware/ESP32_2/ESP32_2.ino
grep -q 'Síťové připojení W5500' "$SKETCH"
grep -q 'IPAddress(link.octets).toString()' "$SKETCH"
grep -q 'stav hladiny a teploty na /status' "$SKETCH"
printf '%s\n' 'PASS: W5500 DHCP status is visible and UI transport limitation is explicit'

RX=firmware/ESP32_1/ESP32_1.ino
grep -q 'Ethernet W5500' "$RX"
grep -q 'stav hladin na /status' "$RX"
