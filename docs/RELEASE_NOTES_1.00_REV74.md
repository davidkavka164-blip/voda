# HOME WATER CONTROLLER — DEV KIT 1.00 REV74

Datum: 15. 11. 2026

## Změny
- NVS úložiště horní objemové kalibrace po zápisu zpětně načítá a porovnává celý významový obsah: absolutní kotvu, všechny aktivní kalibrační body a obě hodnoty rozpracované transferové reference.
- Test ověřuje zachování transferové reference, přesné načtení kalibračních bodů, odmítnutí neplatného zápisu a to, že neúspěšné načtení nezmění výstupní objekt volajícího.
- Pokud zápis nebo zpětné ověření selže, firmware již netvrdí, že původní záznam určitě zůstal zachován. Zneplatní běhový stav objemové kalibrace a zablokuje další kalibrační operace do restartu a ověřeného načtení NVS.
- Chování se týká pouze informativní objemové kalibrace. Nemění elektrickou kalibraci snímače, ochranu čerpadla ani řízení ventilů.
- Aktualizován master, implementační stav, README a statická integrační kontrola.
- W5500 laboratorní sketch a jeho testovací postup zůstávají beze změny; web ESP32 #2 je stále dostupný pouze přes servisní AP.

## Testy
- `./tests/run_native_tests.sh`: 77/77 nativních C++ programů PASS a všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 75/75 ASan/UBSan programů PASS a statické kontroly zahrnuté v ASan runneru PASS.
- ZIP integrita: ověřena po vytvoření balíčku.

## Omezení
- Arduino cílový překlad nebyl proveden; přesná verze ESP32 core a Ethernet knihovny není potvrzena.
- Fyzické ověření NVS, DS18B20, W5500/AP a horní objemové kalibrace nebylo provedeno.
- Produkční web ESP32 #2 zůstává pouze na servisním AP; W5500 HTTP je zatím izolovaný laboratorní sketch.
