# HWC — kontrakt kompatibility HTTP adaptéru REV88

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV87  
Rozsah: pouze statické mapování současného HTTP API obou jednotek a požadavků na budoucí transportní adaptér.

## 1. Ověřený rozsah rout

| Jednotka | GET | POST | Celkem |
|---|---:|---:|---:|
| ESP32 #1 | 1 | 16 | 17 |
| ESP32 #2 | 1 | 10 | 11 |
| **Celkem** | **2** | **26** | **28** |

Každý z 28 registrovaných route callbacků přímo volá příslušnou bezpečnostní bránu (`requireWebAuth()` nebo `requireLocalAuth()`). Obě jednotky mají také autentizovanou větev `onNotFound`. Obě OTA routy `/update` registrují samostatný upload callback a dokončovací callback.

Úplný výpis callbacků je v konzolovém výstupu nástroje `tools/audit_http_server_surface.py`.

## 2. Metody současného WebServer API

Přímé použití v `.ino` vyžaduje zachovat následující rozhraní:

- registrace: `on()`, `onNotFound()`, `collectHeaders()`, `begin()`, `handleClient()`;
- request: `method()`, `hasArg()`, `arg()`, `hasHeader()`, `header()`;
- klient: `client().remoteIP()`;
- autentizace: `authenticate()`, `requestAuthentication()`;
- odpověď: `send()`;
- upload: `upload()` a typ `HTTPUpload` s událostmi START, WRITE, END a ABORTED.

Bezpečnostní funkce `isSameOriginPost()` jsou v současnosti deklarovány s parametrem `WebServer&`. Samotné nahrazení globální proměnné novým typem proto nestačí; tuto vazbu je nutné odstranit nebo zajistit přesně kompatibilní adaptér.

## 3. Požadavky na ekvivalentní chování

1. Zachovat přesné rozlišení HTTP GET/POST a cest; neznámá cesta musí projít stejnou autentizační bránou jako dnes.
2. Načíst query parametry i tělo `application/x-www-form-urlencoded`; zachovat chování `hasArg()` a `arg()` pro formuláře.
3. Zachovat hlavičky `Origin` a `Host` bez normalizace, která by mohla přijmout neplatný/cizí origin.
4. Zachovat autentizační hlavičku Basic Auth, odpověď 401, IP adresu klienta a rate limiter.
5. Pro multipart OTA zachovat sekvenci START → nula nebo více WRITE → END, případně ABORTED; bloky musí být streamované a nesmí se načíst celý firmware do RAM.
6. Při odpojení klienta, chybě čtení, neúplném těle nebo chybném Content-Length vyvolat abort OTA; nikdy nepovažovat nedokončený upload za úspěšný.
7. Odpovědi musí být korektní HTTP/1.1 nebo HTTP/1.0, včetně Content-Length nebo bezpečného close-delimited chování, a spojení se musí zavřít v každém chybovém průchodu.
8. Časové limity parsování a maximální velikosti request line, hlaviček a běžného formulářového těla musí být omezené. Čekání na síť nesmí blokovat ochrannou smyčku čerpadel/ventilů.
9. Wi-Fi STA/AP a W5500 Ethernet musí fungovat nezávisle; klient jednoho rozhraní nesmí blokovat obsluhu druhého.

## 4. Bezpečnější pořadí implementace

1. Nejprve vytvořit a hostitelsky otestovat transportně nezávislý parser HTTP requestu a formulářových parametrů.
2. Samostatně otestovat bezpečnostní rozhodnutí pro Basic Auth, Origin/Host, rate limit a chybové odpovědi.
3. Teprve potom vytvořit adaptér pro `WiFiClient` a `EthernetClient`, který poskytne stávajícím callbackům stejný kontrakt.
4. Zvlášť implementovat a testovat multipart OTA stream; běžné GET/POST nesmí být považovány za důkaz funkční OTA.
5. Po zapojení porovnat všech 28 rout přes obě rozhraní a provést cílový build i fyzické testy.

Nedoporučuje se přepisovat aplikační handlery na druhou kopii pro Ethernet. Dvě kopie by časem mohly mít rozdílné validační a bezpečnostní chování.

## 5. Kompatibilita vývojového prostředí

Projekt je vyvíjen v Arduino IDE 1.8.19 na Windows 7 SP1. Dokumentace projektu uvádí Arduino-ESP32 1.0.6 jako poslední pravděpodobně funkční řadu pro toto prostředí, ale přesná verze instalovaného balíčku stále není potvrzena. Adaptér proto musí být napsán v C++11 a nesmí bez ověření používat API z novějšího Arduino-ESP32 core.

Před cílovým překladem musí být ověřeno přesné API `HTTPUpload` a `WebServer` z nainstalovaného core. Tato závislost je zvlášť důležitá pro OTA callbacky.

## 6. Ověření REV88

- `tools/audit_http_server_surface.py`: PASS.
- Zjištěno 17 rout ESP32 #1 a 11 rout ESP32 #2.
- Všech 28 route callbacků přímo volá příslušnou autentizační bránu.
- Obě OTA routy mají upload callback.
- `onNotFound` a `handleClient()` jsou přítomné u obou jednotek.

Tato kontrola je statická. Neověřuje HTTP parser, síťový transport, autentizaci v běžícím zařízení ani OTA přes Ethernet.

## 7. Rozsah změn

REV88 nemění produkční firmware, routy, autentizaci, Origin/Host kontrolu, OTA, síťové nastavení, W5500 piny, GPIO, NVS, relé, ventily, čidla, pumpové ochrany ani transferovou logiku. Jde o přesnější integrační kontrakt a opakovatelný audit existujícího HTTP povrchu.
