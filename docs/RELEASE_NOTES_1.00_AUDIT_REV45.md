# HOME WATER CONTROLLER – audit revision REV45

Date: 2026-11-13

## Mixed Wi-Fi/Ethernet peer topology

- Applied the user-confirmed topology: ESP32 #1 uses home Wi-Fi STA; ESP32 #2 uses W5500 Ethernet.
- ESP32 #1 peer receiver changed from `EthernetUDP` to `WiFiUDP` on UDP port 4211. Removed ESP32 #1 W5500/SPI/DHCP startup and recovery code.
- ESP32 #1 opens the UDP listener only while STA is connected to the home LAN; it closes the listener on STA loss and reopens it after reconnect.
- Upper-tank transfer continues to require a fresh, valid peer status frame; a Wi-Fi connection or UDP socket alone cannot permit transfer.
- ESP32 #2 W5500 DHCP and broadcast sender remain unchanged.
- Documented the same-LAN and router/AP broadcast-bridging requirements, guest Wi-Fi/client isolation/VLAN risks, and fallback SoftAP limitation.

## Validation

- 67 native test programs passed.
- Mixed-topology static integration checks passed.
- Python tests, Python compileall, shell syntax, common module sync, and ZIP integrity are checked separately in the release audit.
- Not performed: Arduino-ESP32 target compilation, physical router/AP broadcast test, or physical ESP32 testing.
