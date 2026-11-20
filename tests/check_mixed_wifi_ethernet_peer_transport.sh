#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
RX="firmware/ESP32_1/ESP32_1.ino"
TX="firmware/ESP32_2/ESP32_2.ino"
! grep -Eq 'WiFiUDP|EthernetUDP|peerUdp|PeerStatus|pollPeerWifi|sendPeerStatus' "$RX" "$TX"
grep -Fq '#include <WiFi.h>' "$RX"
grep -Fq 'startEthernet();' "$RX"
grep -Fq 'WebServer webServer(80);' "$RX"
grep -Fq 'startEthernet();' "$TX"
grep -Fq 'serviceApActive = WiFi.softAP(localApName.c_str(), localApPassword.c_str());' "$TX"
grep -Fq 'localWebServer.begin()' "$TX"
echo "PASS: independent Wi-Fi and W5500 network interfaces; no inter-unit communication"
