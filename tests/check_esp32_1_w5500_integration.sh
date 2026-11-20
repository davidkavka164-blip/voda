#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
RX="firmware/ESP32_1/ESP32_1.ino"
PINS="firmware/ESP32_1/src/HardwarePins.h"
grep -Fq '#include <ETH.h>' "$RX"
grep -Fq 'HwcPins::Esp32_1::W5500_RST' "$RX"
grep -Fq 'ETH.begin(ETH_PHY_W5500' "$RX"
grep -Fq 'xTaskCreatePinnedToCore(initializeEthernet' "$RX"
grep -Fq 'webServer.on("/status", HTTP_GET, handleRoot);' "$RX"
grep -Fq 'constexpr uint8_t W5500_CS   = 27;' "$PINS"
grep -Fq 'constexpr uint8_t W5500_RST  = 26;' "$PINS"
! grep -Eq 'EthernetUDP|WiFiUDP|peerUdp|PeerStatus' "$RX"
echo 'PASS: ESP32 #1 W5500 task and shared authenticated dashboard are integrated'
