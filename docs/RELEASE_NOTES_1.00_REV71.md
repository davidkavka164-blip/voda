# HOME WATER CONTROLLER — DEV KIT 1.00 REV71

Datum: 15. 11. 2026

## Změny
- Zachován produkční firmware REV70 bez změny síťového stacku.
- Přidán izolovaný testovací sketch `tools/W5500_DUAL_INTERFACE_DIAGNOSTIC/W5500_DUAL_INTERFACE_DIAGNOSTIC.ino` pro souběžný read-only HTTP GET přes Wi-Fi servisní AP (`WebServer`) a W5500 (`EthernetServer`).
- Testovací sketch používá stávající pinout W5500 ESP32 #2: SCK18, MISO19, MOSI23, CS27, RST26.
- Testovací sketch neobsahuje čerpadla, ventily, NVS, autentizaci, POST ani OTA; není součástí produkčního firmware a není určen pro trvalý provoz.
- Přidán postup stolního ověření a rozhodovací brána v `docs/W5500_DUAL_INTERFACE_BENCH_TEST_REV71.md`.
- Přidána statická kontrola, že testovací sketch zůstává oddělen od produkčního firmware.

## Testy
- `sh tests/run_native_tests.sh`: 77/77 nativních C++ testovacích programů PASS; všechny statické integrační kontroly včetně nové izolace W5500 bench sketch PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 75/75 ASan/UBSan testovacích programů PASS; všechny statické kontroly obsažené v ASan skriptu PASS. Nový bench-sketch statický test je součástí standardního runneru, nikoli ASan runneru.
- Arduino cílový překlad testovacího sketch a fyzické ověření W5500/AP nebyly provedeny.

## Omezení
- Přesná verze Arduino-ESP32 core a knihovny Ethernet z uživatelova IDE není známa.
- Testovací sketch musí být přeložen v Arduino IDE 1.8.19 s uživatelovým skutečným core a knihovnou Ethernet.
- Do úspěšného fyzického testu zůstává produkční web ESP32 #2 dostupný pouze přes servisní AP; W5500 poskytuje DHCP, nikoli potvrzený HTTP přístup.
