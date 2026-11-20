# HOME WATER CONTROLLER — DEV KIT 1.00 REV75

Datum: 15. 11. 2026

## Změny
- Přidán samostatný test `test_nvs_upper_tank_volume_calibration_powercut.cpp` pro kritické body ukládání horní objemové kalibrace.
- Model přerušení před potvrzením zápisu ověřuje, že po restartu zůstane předchozí úplný záznam. Při přerušení zápisu výsledného bodu tak zůstane zachována výchozí transferová reference a postup lze bezpečně opakovat.
- Model úspěšného commitu, po kterém API oznámí chybu, ověřuje, že běžící handler nesmí pokračovat, ale po restartu lze načíst celý nový záznam bez reference. Tím se předchází slepému opakování již dokončené kalibrační operace.
- Test je výslovně hostitelský model atomického nahrazení NVS záznamu; nenahrazuje fyzickou zkoušku výpadku napájení na ESP32.
- Aktualizován standardní a ASan/UBSan runner, master, README a implementační stav.
- Produkční firmware, elektrické zapojení, piny ani řízení čerpadel/ventilů se nemění.

## Testy
- `./tests/run_native_tests.sh`: 78/78 nativních C++ programů PASS a všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 ASan/UBSan programů PASS a statické kontroly obsažené v ASan runneru PASS.
- ZIP integrita: PASS; SHA-256 je uveden u výsledného balíčku.

## Omezení
- Arduino cílový překlad nebyl proveden; přesná verze Arduino-ESP32 core/ESP-IDF a Ethernet knihovny není potvrzena.
- Fyzický test NVS při výpadku napájení, DS18B20, W5500/AP a horní objemové kalibrace nebyl proveden.
- Produkční web ESP32 #2 zůstává pouze na servisním AP; W5500 HTTP je zatím izolovaný laboratorní sketch.
