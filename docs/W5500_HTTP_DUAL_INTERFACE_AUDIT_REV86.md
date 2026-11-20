# HOME WATER CONTROLLER — audit webového rozhraní přes W5500 REV86

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV85

## 1. Výchozí stav ověřený ve zdrojích REV85

- ESP32 #1 používá `Ethernet.h` + `SPI.h` pro W5500 DHCP a obnovu linky. Produkční `WebServer webServer(80)` je samostatná instance Arduino-ESP32 a v hlavní smyčce se obsluhuje přes `webServer.handleClient()`.
- ESP32 #2 používá stejnou koncepci: `Ethernet.h` pro W5500 DHCP a samostatný `WebServer localWebServer(80)` pro servisní SoftAP.
- Dashboard obou jednotek výslovně uvádí, že HTTP ovládání přes W5500 není implementováno.
- Ovládací web není jen statická stránka. Obsahuje Basic Auth, Origin/Host kontrolu všech POST požadavků, NVS změny, nouzové akce a OTA upload. Pouhé zobrazení dashboardu přes Ethernet by proto nebylo plnohodnotné řešení.

## 2. Ověřené kandidátní knihovny

### 2.1 WebServer_ESP32_W5500 (Khoi Hoang)

Dokumentace knihovny uvádí jako předpoklad ESP32 core 2.0.6 nebo novější. Současně jde o jinou integraci W5500 přes ESP-IDF/LwIP, nikoli o doplněk k aktuálnímu `Ethernet.h` a jeho TCP/IP stacku. Projekt je určen pro Arduino IDE 1.8.19+, ale tato informace neznamená kompatibilitu se starým ESP32 core 1.0.6.

Rozhodnutí: nyní nepřidávat. Core 2.0.6+ nelze považovat za dostupný build target v uživatelově známém prostředí Windows 7 SP1 bez samostatného potvrzení změny vývojového prostředí.

### 2.2 SparkFun_WebServer_ESP32_W5500

Repozitář SparkFun výslovně uvádí, že knihovna používá nízkoúrovňové TCP/IP, NETIF, SPI a interrupt metody ESP-IDF, které se nesmějí souběžně používat s Arduino SPI/interrupt metodami. Příklad přepíná mezi režimem SparkFun W5500 a standardní Arduino Ethernet knihovnou restartem; nejde o současný provoz obou stacků. README také upozorňuje, že změny paměti knihovny nejsou odstraněny běžným `ESP.restart()` a před přechodem na standardní Ethernet/SPI se musí volat `.end()`.

Rozhodnutí: nepřidávat vedle stávajícího `Ethernet.h`. Znamenalo by to výměnu síťového stacku, změnu inicializace/recovery a zvláštní zacházení s restartem. Zároveň nebyla prokázána kompatibilita s přesnou verzí core v uživatelově IDE.

## 3. Bezpečnostní a integrační požadavky

Plnohodnotná implementace musí zachovat:

1. Stejné routy a chování webu na ESP32 #1 a #2 přes Wi-Fi/AP i Ethernet.
2. Basic Auth, rate limiter a Origin/Host kontrolu všech stavových POST požadavků na obou transportech.
3. Bezpečné a úplné zpracování OTA uploadu; výpadek klienta nebo Ethernetu nesmí nechtěně potvrdit aktualizaci.
4. Zachování Wi-Fi STA/AP při souběžném Ethernetu na ESP32 #1; servisní AP ESP32 #2 musí zůstat dostupný.
5. DHCP/link recovery nezávislé na lokálních pumpových ochranách, čidlech, ventilech a transferovém watchdogu.
6. Bez automatického přesměrování veřejného internetu na nešifrované HTTP. Vzdálený přístup má být přes VPN.
7. Ověření souběžných klientů, opakovaného připojení, změny DHCP adresy, odpojení kabelu během GET/POST/OTA a restartu.

## 4. Závěr REV86

REV86 je auditní revize bez změny produkčního firmware, GPIO, NVS, relé, ventilů, limitů a ochranné logiky. Zatím nebyl nalezen bezpečně ověřený drop-in způsob, jak zpřístupnit stávající `WebServer` přes současný W5500 `Ethernet.h` stack a přitom zachovat známé omezení vývojového prostředí.

Další implementační krok je podmíněn zjištěním přesné verze `esp32 by Espressif Systems` v Arduino IDE uživatele a rozhodnutím, zda zůstane zachována kompatibilita Windows 7/core 1.0.6, nebo bude možné firmware překládat v novějším prostředí. Do té doby se zachovává ověřenější stav REV85: W5500 poskytuje DHCP/link status, ale ovládací web zůstává na Wi-Fi/AP.

## 5. Zdroje

- `WebServer_ESP32_W5500`: https://github.com/khoih-prog/WebServer_ESP32_W5500 — požadavek ESP32 core 2.0.6+.
- `SparkFun_WebServer_ESP32_W5500`: https://github.com/sparkfun/SparkFun_WebServer_ESP32_W5500 — upozornění na konflikt nízkoúrovkového SPI/interrupt stacku s Arduino core a požadavek `.end()` před přechodem na standardní Ethernet.
- Projektové předpoklady Windows 7 / Arduino IDE 1.8.19 a core 1.0.6 jsou popsány v `docs/BUILD_AND_TOOLING.md`; přesná instalovaná verze core zůstává nepotvrzena.
