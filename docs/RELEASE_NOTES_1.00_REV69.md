# HOME WATER CONTROLLER — DEV KIT 1.00 REV69

Datum: 15. 11. 2026

## Změny
- Ověřen přesně nahraný REV68 ZIP; opraven název vnitřní kořenové složky na REV69.
- Implementováno informativní měření DS18B20 na ESP32 #2 / GPIO4.
- Použito OneWire + DallasTemperature. Vyžaduje právě jedno čidlo rodiny 0x28.
- 12bitový převod běží asynchronně; perioda požadavku 10 s, doba převodu 750 ms, stale limit 15 s, opětovné hledání čidla po 30 s.
- Chybějící/nejednoznačné čidlo, chyba čtení, hodnota mimo rozsah a zastaralý vzorek se zobrazují jako nedostupné.
- Teplota je striktně informativní; neovlivňuje bezpečnostní ochranu čerpadla ani ventily.
- Přidán hostitelský validační modul a statická integrační kontrola.
- Přidán audit kompatibility W5500 web serveru; implementace zůstává odložena do zjištění přesné verze Arduino-ESP32 core.

## Testy
- `sh tests/run_native_tests.sh`: 77/77 nativních C++ testovacích programů PASS; všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- ASan/UBSan: 75/75 testovacích programů PASS; statické kontroly PASS.
- Arduino cílový překlad nebyl proveden.
- Fyzické testy ESP32, DS18B20, pull-up rezistoru a W5500 nebyly provedeny.

## Známá omezení
- Je nutné doinstalovat OneWire a DallasTemperature v Arduino IDE 1.8.19 a ověřit jejich kompatibilitu s přesnou verzí ESP32 core.
- Doporučené zapojení datové linky: pull-up přibližně 4,7 kΩ na 3,3 V, nikoli 5 V. Fyzické zapojení nebylo potvrzeno.
- Web ESP32 #2 je dostupný pouze přes servisní AP. W5500 zatím poskytuje síťové DHCP, nikoli HTTP server.
