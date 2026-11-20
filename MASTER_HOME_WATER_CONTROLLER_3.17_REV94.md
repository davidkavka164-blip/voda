# HOME WATER CONTROLLER – MASTER 3.17 / REV94

Datum: 19. 11. 2026

## Stav projektu
REV94 navazuje na REV93. Zachovává dosavadní architekturu dvou nezávislých ESP32 jednotek a jejich lokální ochrany.

## Síťové řešení – zjednodušená cesta
Cílem je jeden webový server na každé jednotce, dostupný přes Wi‑Fi i W5500. Byla nalezena vhodná cesta pro ESP32 Arduino core 3+: knihovna EthernetESP32 integruje W5500 do síťové abstrakce ESP32. Tím lze potenciálně zachovat stávající `WebServer` a všech 28 rout bez ručního přepisování.

Kompatibilita závisí na verzi ESP32 Arduino core. Produkční firmware se proto zatím nemění. V REV94 je samostatný testovací sketch; viz `docs/W5500_SIMPLE_NETWORK_INTERFACE_POC_REV94.md`.

## Neměnné bezpečnostní zásady
- ESP32 #1 a ESP32 #2 pracují samostatně, bez vzájemné komunikace.
- Čerpadla a ventily mají lokální bezpečnostní logiku.
- Síťové chyby nesmějí povolit čerpadlo ani otevřít ventil.
- Stávající autentizace, Origin/Host kontrola, limity přihlášení a OTA zůstávají zachovány.
- Žádné změny pinů, relé, senzorů, NVS ani přepouštěcí logiky v této síťové etapě.

## Další krok
Zjistit verzi `esp32 by Espressif Systems` v Arduino IDE. Pokud je core 3.x, provést izolovaný test POC. Pokud je core 2.x, nejprve vyhodnotit riziko aktualizace; nepřecházet automaticky na složitý vlastní HTTP server.
