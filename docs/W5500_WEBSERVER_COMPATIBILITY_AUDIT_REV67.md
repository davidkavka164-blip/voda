# W5500 / web server compatibility audit — REV67

## Current implementation

ESP32 #2 currently uses the Arduino `Ethernet` library with W5500 over SPI for DHCP and link recovery. Its HTTP interface is an Arduino-ESP32 `WebServer` instance started on the local Wi-Fi SoftAP. A DHCP lease from the SPI Ethernet library does not make that `WebServer` reachable over the W5500 interface.

## Candidate library findings

- `WebServer_ESP32_W5500` documents support for ESP32 with W5500 through the ESP-IDF/LwIP W5500 Ethernet integration and lists ESP32 core 2.0.6+ as a prerequisite: https://github.com/khoih-prog/WebServer_ESP32_W5500
- The related SparkFun library uses a different W5500 integration path and its documentation warns about SPI/interrupt interactions and incompatibility with Arduino-ESP32 v3.0: https://github.com/sparkfun/SparkFun_WebServer_ESP32_W5500
- These are not drop-in additions to the current `Ethernet.h` + SPI `Ethernet.begin()` implementation. They change the Ethernet/network stack and must be assessed for SoftAP coexistence, OTA, DHCP recovery, GPIO/SPI ownership, and the exact installed core version.

## Decision in REV67

No web-server library or Ethernet stack was replaced. The exact installed ESP32 core version on the user's Windows 7 / Arduino IDE 1.8.19 setup is not confirmed. Introducing a library requiring core 2.0.6+ without that information would risk breaking the existing firmware build and W5500 recovery. ESP32 #2 web remains AP-only until the core version and a compatible dual-interface design are verified.

## Next verification

1. Confirm the installed `esp32 by Espressif Systems` version in Arduino Boards Manager (without resetting or changing the router).
2. Select a W5500 web-server approach compatible with that exact core and the existing hardware wiring.
3. Verify simultaneous service AP and W5500 HTTP access, authentication/CSRF, OTA upload, reconnect behavior, and that local sensing/pump protection continue during Ethernet failure.
4. Perform an Arduino target build and hardware tests before declaring LAN web access available.
