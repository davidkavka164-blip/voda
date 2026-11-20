# W5500 – audit webového přístupu ESP32 #2 (REV70)

## Současný stav v projektu

ESP32 #2 používá `Ethernet.h` a `Ethernet.begin()` pro W5500. Ethernet vrstva získává DHCP adresu a má vlastní kontrolu linky a opakování DHCP. HTTP rozhraní je vytvořeno objektem `WebServer localWebServer(80)` a spuštěno po přepnutí Wi-Fi do režimu `WIFI_AP`; obsluha klientů probíhá přes `localWebServer.handleClient()`. V této konfiguraci je potvrzen pouze servisní AP. DHCP adresa W5500 sama o sobě nedokazuje, že stejný `WebServer` přijímá klienty přes Ethernet.

REV70 doplňuje do servisní stránky informaci o stavu W5500 a přidělené IP. Text stránky výslovně rozlišuje DHCP od dostupnosti HTTP přes LAN.

## Zhodnocené směry

### 1. WebServer_ESP32_W5500 / SparkFun varianta

Knihovna `WebServer_ESP32_W5500` uvádí požadavek Arduino-ESP32 core 2.0.6 nebo novější. To nelze v projektu předpokládat, protože uživatel zatím nepotvrdil přesnou verzi core. SparkFun varianta navíc výslovně upozorňuje na konflikty s Arduino SPI/interrupt obsluhou a na nutnost pečlivě řídit životní cyklus W5500. Proto ji nelze bez cílového překladu a zkoušek přidat vedle současné `Ethernet.h` implementace.

Zdroje:
- https://github.com/khoih-prog/WebServer_ESP32_W5500
- https://github.com/sparkfun/SparkFun_WebServer_ESP32_W5500
- https://github.com/espressif/arduino-esp32/issues/7909

### 2. EthernetWebServer

Knihovna EthernetWebServer deklaruje podporu W5x00 a ESP32 a uvádí použití Ethernet_Generic jako výchozí vrstvy W5x00 s vlastním SPI. To z ní dělá kandidáta pro samostatný Ethernet server, ale ne pro automatické sdílení existujícího objektu `WebServer`. Před použitím by bylo nutné ověřit konkrétní verzi, konflikt/kompatibilitu s aktuální knihovnou `Ethernet.h`, API pro upload OTA, Basic Auth, hlavičky Origin/Host a parsování formulářů. Licence MIT je příznivá, ale sama neřeší kompatibilitu.

Zdroj:
- https://github.com/khoih-prog/EthernetWebServer

### 3. Nativní NetworkServer / WebServer

Novější Arduino-ESP32 má obecné `NetworkServer`/`NetworkClient` abstrakce. Ty jsou navrženy pro síťové rozhraní spravované lwIP. Současný projekt ale používá W5500 přes knihovnu `Ethernet.h`, která obsluhuje TCP/IP sockety přímo v čipu W5500; nelze bez ověření předpokládat, že se tato cesta objeví jako lwIP síťové rozhraní pro `WebServer`.

Zdroj:
- https://docs.espressif.com/projects/arduino-esp32/en/latest/api/network.html
- https://github.com/espressif/arduino-esp32/blob/master/libraries/WebServer/src/WebServer.h

## Doporučené rozhodnutí

Zachovat současnou funkční kombinaci `Ethernet.h` + servisní `WebServer` na AP a neprovádět zatím výměnu Ethernet stacku ani přidání knihovny s neověřenou kompatibilitou.

Před implementací webu přes W5500 jsou nutné tyto kroky:
1. Zjistit přesnou verzi Arduino-ESP32 core instalovanou v Arduino IDE uživatele.
2. Zaznamenat verze knihoven Ethernet, OneWire a DallasTemperature.
3. Vybrat jedinou kandidátní Ethernet HTTP vrstvu a ověřit ji na odděleném minimálním sketchi se stejným W5500 modulem.
4. Ověřit souběh W5500 HTTP a servisního AP; nejprve pouze GET stránku bez výstupů.
5. Teprve poté přenést stejné chráněné routy včetně POST, Origin/Host, Basic Auth a OTA uploadu. U OTA musí být ověřeno, že obě cesty sdílejí bezpečnostní kontroly a blokace nouzového režimu.
6. Při výpadku Ethernetu nesmí dojít k resetu ani blokování lokálního měření a ochrany čerpadla.

## Stav REV70

- DHCP/IP W5500 je viditelné v servisním AP dashboardu.
- HTTP přes W5500/LAN není implementováno ani prohlášeno za funkční.
- Servisní AP a stávající HTTP routy zůstávají beze změny.
- Přesná verze core a fyzická revize W5500 modulu nejsou známy.
