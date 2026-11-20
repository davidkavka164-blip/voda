# HOME WATER CONTROLLER – audit revision REV48

Date: 2026-11-13

## Scope

Documentation-only audit of the gap between the existing peer security modules and the actual ESP32 UDP sketches. No firmware, network topology, pinout, NVS layout, eFuse or runtime behavior was changed.

## Findings

- The mixed Wi-Fi STA (ESP32 #1) / W5500 Ethernet (ESP32 #2) UDP status transport is implemented.
- Current peer status remains CRC-only. ESP32 #1 does not run the handshake from its UDP receive loop; ESP32 #2 binds UDP port 4212 but does not parse incoming handshake packets.
- HMAC, handshake, timed retry, directional session codec and replay guard modules exist and pass host tests, but are not connected to the `.ino` transport call sites.
- The authenticated-control flag remains false; normal/service upper-source valve control stays blocked.
- No production peer-secret backend has been verified to provide encrypted/authenticated storage. Plain Preferences/NVS is not accepted as a secure secret vault.
- The existing NVS replay journal is a two-slot Preferences journal with read-back and fail-closed corruption handling; it does not provide confidentiality or physical flash rollback protection.

## Next implementation sequence

1. Confirm exact Arduino-ESP32 core version and decide a verifiable secure-secret storage mechanism.
2. Complete physical provisioning of the same 32-byte peer secret without serial/web/log exposure.
3. Integrate bidirectional HELLO/CHALLENGE/FINISH/READY into existing UDP sockets and add a canonical authenticated status frame.
4. Integrate authenticated replay/freshness checks and only then remove the temporary upper-transfer block.

## Tests rerun

- Standard native tests: 67/67 PASS.
- ASan/UBSan: 65/65 PASS.
- Python provisioning client: 17/17 PASS.
- Arduino target build and physical tests: not performed.
