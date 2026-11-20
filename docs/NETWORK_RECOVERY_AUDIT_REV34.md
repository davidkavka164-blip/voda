# Network recovery audit — REV34

> Historical snapshot. Its assumption that both ESP32 units use W5500 for peer UDP is superseded by the user-confirmed mixed Wi-Fi/Ethernet topology implemented in REV45. See `NETWORK_TOPOLOGY_AUDIT_REV45.md`.

## Confirmed interface roles

### ESP32 #1 (main controller)
- The web UI is served by `WebServer` on the ESP32 Wi-Fi interface.
- With saved home Wi-Fi credentials, startup tries station mode for up to 15 seconds.
- If startup connection fails, the unit starts its configuration AP in AP+STA mode and continues retrying the saved station connection.
- During a later Wi-Fi outage, the unit retries the saved network every 15 seconds. If the outage lasts 30 seconds, it starts the fallback AP in AP+STA mode and continues station retries.
- If Wi-Fi reconnects while fallback AP is running, the current code leaves the AP running until a restart. This preserves local service access and does not affect the web server.
- If no saved SSID exists, the unit starts the configuration AP directly.

### ESP32 #2 (upper tank)
- The local web UI is served from its own Wi-Fi SoftAP.
- It does not join the home Wi-Fi as a station in this revision. Its web UI is therefore local to that AP.
- Loss of W5500 peer Ethernet does not disable local sensing, local pump protection, or the local AP/web UI.

### W5500 Ethernet on both units
- W5500 is used for DHCP and UDP peer-status transport (ports 4211/4212), not as a web-server interface.
- The current firmware does not create an `EthernetServer` or `EthernetWebServer`. Therefore a working W5500 cable does not provide an alternate route to the web UI when ESP32 #1 Wi-Fi is unavailable.
- Both W5500 devices are expected to be on the same local DHCP LAN for peer UDP broadcast. The physical topology and router broadcast/client-isolation behavior still require a real installation test.
- The firmware does not itself expose the web UI to the public internet. Remote access requires a separately configured secure network path (for example, a VPN on the home router); port forwarding of the unauthenticated HTTP service is not recommended.

## Change in REV34

The previous DHCP retry path called `Ethernet.begin(mac, 5000, 1000)` synchronously from the main control loop. That could hold the loop for approximately five seconds during a W5500 recovery attempt. Since sensor freshness and pump-relay synchronization are also serviced in that loop, this was an unnecessarily long pause.

- Initial W5500 setup during `setup()` retains a 5-second DHCP timeout; pump safety outputs have already been initialized LOW and the control loop has not started.
- W5500 retries initiated during normal operation now use a 1-second DHCP timeout and a 250 ms response timeout.
- Both units keep their existing 60-second retry cadence when no usable link/DHCP lease is available, and retry immediately after a detected physical link restoration.
- This bounds the blocking time of an in-loop DHCP retry; it does not make DHCP asynchronous.

## Safety behavior retained

- ESP32 #1 web servicing and Wi-Fi reconnect logic continue independently of W5500 peer availability.
- ESP32 #2 local sensor acquisition and pump protection continue independently of W5500 peer availability.
- The upper transfer path remains unavailable when peer status is missing or stale; it does not infer upper-tank state from Ethernet link alone.
- A W5500 recovery attempt does not regenerate peer boot identity or reset the peer sequence counter.

## Verification limits

Host-side static checks and native tests cannot verify actual Wi-Fi association, SoftAP coexistence, DHCP server timing, W5500 link recovery, UDP broadcast delivery, or mobile browser access. Those require the installed router, both W5500 modules, and physical ESP32 boards. Arduino-ESP32 target compilation remains unperformed in the current environment.
