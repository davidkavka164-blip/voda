# HOME WATER CONTROLLER — DEV KIT 1.00 REV73

Datum: 15. 11. 2026

## Změny
- Zachován neblokující stavový parser izolovaného W5500/AP diagnostického sketch z REV72.
- Opraveno chování horní objemové kalibrace při otevřené transferové referenci. Ruční editace kalibračních bodů nyní není povolena, dokud uživatel transferovou kalibraci nedokončí nebo referenci výslovně nezruší.
- Ochrana je v modelu, POST handleru `/volume-point` i webovém rozhraní.
- Nativní test ověřuje, že pokus o ruční změnu během reference vrátí konflikt a reference zůstane zachována.
- `UpperTankVolumeCalibration.cpp` je synchronizován ve společném zdroji i obou ESP32 Arduino `src` stromech.
- Aktualizován master, implementační stav, README a návod W5500 testu.

## Testy
- `./tests/run_native_tests.sh`: 77/77 nativních C++ programů PASS a všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 75/75 ASan/UBSan programů PASS a statické kontroly zahrnuté v ASan runneru PASS.
- ZIP integrita: PASS.

## Omezení
- Arduino cílový překlad nebyl proveden; přesná verze ESP32 core a Ethernet knihovny není potvrzena.
- Fyzické ověření DS18B20, W5500/AP a horní objemové kalibrace nebylo provedeno.
- Produkční web ESP32 #2 zůstává pouze na servisním AP; W5500 HTTP je zatím izolovaný laboratorní sketch.
