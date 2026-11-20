#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
RX="firmware/ESP32_1/ESP32_1.ino"
TX="firmware/ESP32_2/ESP32_2.ino"
! grep -Eq 'WiFiUDP|EthernetUDP|peerUdp|PeerStatus|pollPeerWifi|sendPeerStatus' "$RX" "$TX"
grep -Fq '#include <WiFi.h>' "$RX"
grep -Fq 'ethernetStatus.begin(adminPassword);' "$RX"
grep -Fq 'WebServer webServer(80);' "$RX"
grep -Fq 'ethernetStatus.begin(localAdminPassword);' "$TX"
grep -Fq 'WiFi.mode(WIFI_AP)' "$TX"
grep -Fq 'localWebServer.begin()' "$TX"
echo "PASS: independent Wi-Fi and W5500 network interfaces; no inter-unit communication"
