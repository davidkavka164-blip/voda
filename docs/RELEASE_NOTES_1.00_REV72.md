# HOME WATER CONTROLLER — DEV KIT 1.00 REV72

Datum: 15. 11. 2026

## Změny
- Produkční firmware obou ESP32 zůstává beze změny.
- Izolovaný sketch `tools/W5500_DUAL_INTERFACE_DIAGNOSTIC/W5500_DUAL_INTERFACE_DIAGNOSTIC.ino` nyní používá neblokující stavový parser Ethernet HTTP.
- Parser používá pevný buffer request line 96 B, buffer hlavičky 128 B, nejvýše 15 neprázdných hlaviček, timeout 500 ms a nejvýše 64 přijatých bajtů na průchod smyčkou.
- Přijímá pouze `GET / HTTP/1.0` a `GET / HTTP/1.1`; ostatní nebo neúplné požadavky odmítá.
- Díky neblokujícímu příjmu se v každém průchodu dále obsluhuje `WebServer` servisního AP.
- Statická kontrola ověřuje pevné limity a nepřítomnost `readStringUntil()` / `client.setTimeout()`.
- Opraveno právo spuštění `tests/run_native_tests.sh`; přímé spuštění funguje.
- Aktualizován master, README a návod laboratorního testu.

## Testy
- `./tests/run_native_tests.sh`: 77/77 nativních C++ programů PASS a všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 75/75 ASan/UBSan programů PASS a statické kontroly zahrnuté v ASan runneru PASS.
- ZIP integrita: PASS.

## Omezení
- Arduino cílový překlad nebyl proveden; přesná verze ESP32 core a Ethernet knihovny není potvrzena.
- Fyzický souběh Wi-Fi AP + W5500 HTTP nebyl ověřen.
- Produkční web ESP32 #2 zůstává pouze na servisním AP. Diagnostický sketch není produkční server a není určen pro trvalý provoz.
