# HOME WATER CONTROLLER — DEV KIT 1.00 REV76

Datum: 15. 11. 2026

## Změny
- Přidán autentizovaný servisní postup `/volume-corrupt-reset` pro odstranění pouze poškozeného záznamu horní objemové kalibrace.
- Úložiště nejprve celý záznam přečte a ověří jeho neplatnost; platný záznam odmítne odstranit. Při chybě/krátkém čtení nebo nejednoznačné délce reset odmítne, protože samotné selhání čtení není důkazem poškození.
- Po odstranění se ověřuje nepřítomnost klíče. Při chybě nebo nejednoznačném výsledku zůstává běhová kalibrace zablokována do restartu a opětovného načtení.
- Web výslovně upozorňuje na úplnou ztrátu kalibračních bodů a povinnost zadat nový fyzicky ověřený absolutní kotevní bod.
- Servisní reset se týká výhradně informační objemové kalibrace; nemění elektrickou kalibraci snímače, limity, ochrany čerpadla ani ventily.
- Doplněny testy ochrany platného záznamu a přerušení / nejednoznačného výsledku odstranění.

## Testy
- `./tests/run_native_tests.sh`: 78/78 nativních C++ programů PASS a statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 ASan/UBSan programů PASS a statické kontroly PASS.
- ZIP integrita a SHA-256 ověřeny.

## Omezení
- Arduino cílový překlad nebyl proveden; přesná verze Arduino-ESP32 core / ESP-IDF a Ethernet knihovny není potvrzena.
- Fyzické testy NVS, brownout, DS18B20, W5500/AP a objemové kalibrace nebyly provedeny.
- Produkční web ESP32 #2 zůstává pouze na servisním AP; W5500 HTTP je izolovaný laboratorní sketch.
