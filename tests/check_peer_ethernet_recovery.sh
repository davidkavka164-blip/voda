#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TX="firmware/ESP32_2/ESP32_2.ino"
grep -Fq 'ETH.linkUp()' "$TX"
grep -Fq 'ETH.hasIP()' "$TX"
grep -Fq 'startEthernet();' "$TX"
grep -Fq 'if (ETH.begin(ETH_PHY_W5500' "$TX"
! grep -Eq 'EthernetUDP|peerUdp|sendPeerStatus|PeerStatus' "$TX"
RX="firmware/ESP32_1/ESP32_1.ino"
! grep -Eq 'WiFiUDP|peerUdp|pollPeerWifi|PeerStatus' "$RX"
grep -Fq 'xTaskCreatePinnedToCore(initializeEthernet' "$TX"
grep -Fq 'attempt <= 3' "$TX"
echo "PASS: ESP32 #2 W5500 internet recovery is independent; no peer UDP transport remains"
