#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
TX="firmware/ESP32_2/ESP32_2.ino"
grep -Fq 'Ethernet.linkStatus()' firmware/common/EthernetStatus.h
grep -Fq 'Ethernet.linkStatus() == LinkOFF' firmware/common/EthernetStatus.h
grep -Fq 'ethernetStatus.begin(localAdminPassword);' "$TX"
grep -Fq 'Ethernet.begin(mac, 2000, 250)' firmware/common/EthernetStatus.h
! grep -Eq 'EthernetUDP|peerUdp|sendPeerStatus|PeerStatus' "$TX"
RX="firmware/ESP32_1/ESP32_1.ino"
! grep -Eq 'WiFiUDP|peerUdp|pollPeerWifi|PeerStatus' "$RX"
echo "PASS: ESP32 #2 W5500 internet recovery is independent; no peer UDP transport remains"
