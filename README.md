# HOME WATER CONTROLLER — vývojový balíček 1.00 REV97

REV97 upravuje čtení HTTP hlaviček prohlížečů na ethernetovém přehledu obou jednotek. Aktualizace OTA se spouští z ovládacího webu přes Wi-Fi.

Aktuální stav, závazné zásady a otevřené zkoušky jsou v [masteru 3.18 / REV97](MASTER_HOME_WATER_CONTROLLER_3.18_REV97.md).

## Otevření v Arduino IDE

| Jednotka | Sketch | Funkce |
|---|---|---|
| ESP32 #1 | [ESP32_1.ino](firmware/ESP32_1/ESP32_1.ino) | Studna, dolní nádrž a přepouštění |
| ESP32 #2 | [ESP32_2.ino](firmware/ESP32_2/ESP32_2.ino) | Horní nádrž a teplota vody |

Rozbal celý projekt. Arduino IDE 1.8.19 otevře soubor `.ino` v odpovídající složce včetně jejího podadresáře `src`. Deska je **ESP32 Dev Module** a cílový balíček je **esp32 by Espressif Systems 1.0.6**. Pro obě jednotky použij knihovnu **Ethernet 2.0.2**. ESP32 #2 navíc používá **OneWire 2.3.8** a **DallasTemperature 4.0.6**.

## Síť a místní správa

| Rozhraní | ESP32 #1 | ESP32 #2 |
|---|---|---|
| Domácí Wi-Fi (STA) | Ovládací web. Údaje se nastavují ve webu a ukládají v lokálním NVS. | Ovládací web. Údaje se nastavují ve webu a ukládají v lokálním NVS. |
| Servisní Wi-Fi AP | Ovládací web. AP se spustí bez nastavené domácí Wi-Fi nebo při jejím výpadku. | Ovládací web. AP běží také při připojení k domácí Wi-Fi. |
| W5500 (DHCP) | Přihlášený přehled hladin studny a dolní nádrže na `/status`. | Přihlášený přehled hladiny horní nádrže a teploty na `/status`. |

Ethernetová stránka používá účet `admin` dané jednotky. Ovládací formuláře a OTA jsou na Wi-Fi. Každá jednotka spravuje vlastní připojení, měření a ochrany. Na obou jednotkách má W5500 zapojení **CS 27, RESET 26, SCK 18, MISO 19, MOSI 23**.

## Ověření

Překlad obou sketchů pro **ESP32 Dev Module** ověřen s Arduino-ESP32 1.0.6 a výše uvedenými knihovnami.

Na ESP32 #1 je ověřen servisní AP, ovládací web z domácí Wi-Fi a ethernetový přehled v REV97 včetně návratu po odpojení kabelu. Uživatel potvrdil i úspěšné nahrání REV97 přes OTA na ESP32 #1. Zkoušky ESP32 #2 jsou vedené v masteru.

## Historické poznámky k dřívějším revizím

### Vývojový balíček 1.00 REV90

Datum: 19. 11. 2026

### Stav v době MASTER 3.13 / REV90

REV89 navazuje na audit REV86, TUV HTTP audit REV87 a kontrakt adaptéru REV88. Přidává samostatný pevně omezený C++11 parser běžných HTTP požadavků, který je hostitelsky testovatelný a nezávislý na síťovém klientovi.

**Aktuální funkční rozsah:** W5500 poskytuje DHCP a obnovu linky; Wi-Fi STA/AP zůstává zachována; webové ovládání je dostupné pouze přes Wi-Fi/AP. Ethernetová HTTP obsluha zatím není implementována. Nový parser není zapojen do produkčních `.ino`; autentizace, POST handlery a OTA proto zůstávají beze změny.

Master v době REV90: `MASTER_HOME_WATER_CONTROLLER_3.13_REV90.md`. Technický kontrakt: `docs/HTTP_SERVER_ADAPTER_CONTRACT_REV88.md`. Popis parseru: `docs/HTTP_REQUEST_PARSER_STAGE_REV89.md`.

## Ověření

- Přidáno 80 hostitelských nativních testovacích programů; všechny prošly.
- Statické integrační kontroly celé sady také prošly.
- Produkční `.ino`, routy, přihlášení, Origin/Host kontrola, OTA, GPIO, NVS a bezpečnostní řízení zůstávají beze změny.
- Arduino cílový překlad ani fyzický test Ethernetového HTTP nebyl proveden.
- Přesná verze ESP32 core v uživatelově Arduino IDE zůstává nepotvrzena.


## REV87 – TUV Ethernet webserver adaptation audit

Added `docs/TUV_HTTP_SERVER_ADAPTATION_AUDIT_REV87.md`. The uploaded TUV project confirms the dual `WiFiServer`/`EthernetServer` transport pattern, but its HTTP parser and security model are not a safe drop-in for HWC. No production firmware, GPIO, NVS, pump, valve, or transfer logic was changed in this audit revision.

## REV88 — HTTP adapter contract

REV88 adds `MASTER_HOME_WATER_CONTROLLER_3.11_REV88.md`, the detailed HTTP adapter contract in `docs/HTTP_SERVER_ADAPTER_CONTRACT_REV88.md`, and the repeatable route/auth inventory tool `tools/audit_http_server_surface.py`. This is an audit/tooling revision; production firmware and Ethernet HTTP serving are unchanged.

## REV89 — HTTP parser stage

`firmware/common/HttpRequestParser.h` is mirrored into both ESP32 sketch source trees. It handles bounded HTTP/1.x GET/POST parsing and URL-encoded forms only. It is not connected to the live web server; OTA/multipart streaming, transport reads, authentication and response writing remain future work.


## REV90 — transportní integrační brána

REV90 přesně zmapovala skutečné závislosti obou produkčních sketch na Arduino `WebServer`: 17 rout ESP32 #1 a 11 rout ESP32 #2. Přidává `tools/audit_http_transport_contract.py` a `docs/HTTP_TRANSPORT_INTEGRATION_GATE_REV90.md`. Tyto kontroly brání ztrátě rout, autentizačních hooků a OTA upload callbacku při budoucím přepisu. Produkční firmware se nemění; HTTP přes W5500 stále není implementováno.


## REV93 — síťová část zjednodušena

Viz `MASTER_HOME_WATER_CONTROLLER_3.16_REV93.md` a `docs/NETWORK_SIMPLIFICATION_DECISION_REV93.md`. Firmware zůstává beze změny.


## REV94 – zjednodušení Ethernet webu

Přidán izolovaný testovací sketch pro ověření, zda lze s ESP32 core 3+ použít jediný WebServer současně přes Wi-Fi a W5500. Produkční firmware se nemění. Podrobnosti: `docs/W5500_SIMPLE_NETWORK_INTERFACE_POC_REV94.md`.
