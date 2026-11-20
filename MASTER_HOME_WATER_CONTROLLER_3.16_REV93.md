# HOME WATER CONTROLLER — MASTER 3.16 / REV93

Datum: 19. 11. 2026
Výchozí archiv: REV92 (ověřený ZIP, 775 položek).

## Zjednodušení síťové části

Cílem není vytvářet nový obecný webový framework. Jde o lokální řídicí systém vody. Nesmí vzniknout druhá kopie 28 HTTP rout ani nový parser, pokud to není nezbytné.

### Zjištěný stav ve zdrojích REV92

- ESP32 #1 i ESP32 #2 používají Arduino `WebServer` pro HTTP.
- Obě jednotky zároveň obsahují `#include <Ethernet.h>` a volání `Ethernet.begin(...)` pro W5500.
- `WebServer` není v současném kódu automaticky připojen k `EthernetServer` z knihovny `Ethernet.h`.
- Proto přítomnost funkčního W5500/IP adresy sama o sobě neznamená, že webové stránky a ovládací routy odpovídají i na Ethernetu.
- Každá jednotka má vlastní webové routy; ESP32 #1 a #2 nejsou z hlediska obsahu totožné.

### Závazný směr další práce

1. Nepřepisovat všech 28 rout do vlastního HTTP parseru.
2. Nepřidávat druhou implementaci autentizace, CSRF ani OTA.
3. Nejprve ověřit, zda použitá verze ESP32 Arduino core dovoluje W5500 připojit jako nativní lwIP síťové rozhraní (ETH/W5500), které může sdílet standardní TCP/IP server.
4. Pokud ano, použít jediný `WebServer` a zachovat stávající handlery. Wi-Fi STA/AP a W5500 budou pouze síťová rozhraní; žádná druhá sada rout.
5. Pokud aktuální core / W5500 driver tuto cestu nepodporuje, ponechat současný stabilní Wi-Fi webserver a nepřidávat složitý paralelní HTTP framework. Ethernetovou webovou dostupnost odložit do rozhodnutí o kompatibilní verzi core/driveru.
6. Žádná síťová chyba nesmí ovlivnit ochranné výstupy čerpadel ani bezpečnost ventilů. Síťové funkce zůstávají neblokující.

### Co se v REV93 mění

Pouze dokumentace a plán integrace. Produkční `.ino`, routy, síťový driver, piny, čerpadla, ventily a ochranná logika zůstávají beze změny. Záměrně se nepřidává další parser ani abstraktní transportní framework.

### Ověření

Archiv REV92 byl před úpravou rozbalen a kontrola ZIP vrátila `testzip() = None`, 775 položek. V REV93 nebyl proveden Arduino target build ani fyzický test W5500. Neuvádí se žádné nové PASS výsledky softwarových testů, protože se neměnil firmware.
