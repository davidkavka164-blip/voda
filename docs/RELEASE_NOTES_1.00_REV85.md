# HOME WATER CONTROLLER — DEV KIT 1.00 REV85

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV84

## Změna

ESP32 #1 nově inicializuje W5500 přes SPI na vyhrazených pinech z existující mapy:
- SCK GPIO18, MISO GPIO19, MOSI GPIO23
- CS GPIO27, RESET GPIO26

W5500 získává IPv4 konfiguraci přes DHCP. Firmware sleduje fyzický link a platnost IP adresy, při výpadku samostatně obnovuje DHCP (při přítomném linku nejpozději po 60 s). Síťová obnova neběží jako podmínka lokálního měření, pumpových ochran ani transferů. Wi-Fi STA a konfigurační AP zůstávají zachovány.

Dashboard ESP32 #1 zobrazuje stav W5500 a jeho IP adresu. Výslovně upozorňuje, že produkční ovládací web je v REV85 dostupný pouze přes Wi-Fi STA / konfigurační AP. W5500 HTTP routy, autentizace, POST a OTA nejsou implementovány.

Postup fyzického ověření: `docs/ESP32_1_W5500_TEST_REV85.md`.

## Testy

- Nativní C++ testy: 79/79 PASS.
- Statické integrační kontroly: PASS, včetně nové `check_esp32_1_w5500_integration.sh`.
- Python 17/17 PASS je převzatý výsledek REV84; Python kód se v REV85 neměnil.
- ASan/UBSan 77/77 PASS je převzatý výsledek REV84; v REV85 nebyl znovu spuštěn.
- ZIP integrita se ověří při zabalení.
- Arduino cílový překlad ani fyzické testy W5500 nebyly provedeny.

## Bezpečnostní rozsah

Změna se týká pouze síťové inicializace W5500 ESP32 #1 a zobrazení jejího stavu. Nemění GPIO čidel, relé čerpadel, relé ventilů, prahy ochran, transferovou logiku, NVS konfiguraci ani nouzové režimy. W5500 je síťová doplňková funkce; jeho selhání nesmí zastavit lokální bezpečnostní řízení.
