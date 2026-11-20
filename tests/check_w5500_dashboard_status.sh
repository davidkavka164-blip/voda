#!/bin/sh
set -eu
SKETCH=firmware/ESP32_2/ESP32_2.ino
grep -q 'Síťové připojení W5500' "$SKETCH"
grep -q 'ETH.localIP().toString()' "$SKETCH"
grep -q 'Ovládací web je dostupný přes Ethernet i Wi-Fi' "$SKETCH"
printf '%s\n' 'PASS: W5500 DHCP status is visible on both transport interfaces'

RX=firmware/ESP32_1/ESP32_1.ino
grep -q 'Ethernet W5500' "$RX"
grep -q 'Ovládací web je dostupný přes Ethernet i Wi-Fi' "$RX"
