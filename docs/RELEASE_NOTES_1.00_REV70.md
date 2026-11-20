# HOME WATER CONTROLLER — DEV KIT 1.00 REV70

Datum: 15. 11. 2026

## Změny
- Ověřen přesně nahraný archiv REV69; zachován jeho zdrojový stav jako základ REV70.
- Opravena zavádějící startovní diagnostika: ESP32 #2 nyní vypisuje GPIO4 jako datový pin DS18B20 s informativním měřením, nikoli jako dosud nečtený pin.
- Odstraněno redundantní druhé volání `OneWire/DallasTemperature.begin()` při startu; inicializaci provádí jednotná discovery funkce.
- Rozšířen test teploty o přetečení `millis()` a ověření, že chybné čtení neposouvá čas posledního platného vzorku.
- Servisní dashboard zobrazuje stav DHCP/linky W5500 a IP adresu při přiděleném lease.
- Dashboard výslovně uvádí, že web je stále dostupný pouze přes servisní AP a DHCP neznamená HTTP dostupnost přes LAN.
- Přidán samostatný audit možností W5500 HTTP serveru a statická kontrola dashboardu.

## Testy
- `sh tests/run_native_tests.sh`: 77/77 nativních C++ testovacích programů PASS; všechny statické kontroly včetně nového W5500 dashboard testu PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 75/75 ASan/UBSan testovacích programů PASS; všechny statické kontroly včetně nového W5500 dashboard testu PASS.
- Arduino cílový překlad a fyzické testy nebyly provedeny.

## Omezení
- Neznáme přesnou verzi Arduino-ESP32 core a verzí OneWire/DallasTemperature.
- Fyzické zapojení DS18B20 na GPIO4, pull-up rezistor a vedení nejsou ověřeny.
- Web přes W5500/LAN není implementován; W5500 DHCP stav se pouze zobrazuje v servisním AP.
- Přesná fyzická revize W5500 modulu a zapojení SPI/RST nebyly fyzicky ověřeny.
