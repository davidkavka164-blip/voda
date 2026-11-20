# HWC HTTP transport adapter — implementation design REV91

Datum: 19. 11. 2026  
Východisko: skutečný zdroj REV90 (odvozený z REV89).

## Výsledek detailního rozboru

REV90 správně identifikovala 28 cest, ale samotný seznam API nestačí. Provedený rozbor přímých volání ukázal, že kompatibilní vrstva musí zachovat také následující konkrétní chování:

| Oblast | ESP32 #1 | ESP32 #2 | Požadavek adaptéru |
|---|---:|---:|---|
| `arg()` | 30 | 16 | Zachovat URL decode, pořadí/duplicitní parametry a návratový typ `String` nebo upravit všechny konzumenty explicitně |
| `hasArg()` | 20 | 10 | Rozlišit chybějící parametr od prázdného parametru |
| `send()` | 7 | 44 | Zachovat status, content-type, payload a bezpečné odeslání délky odpovědi |
| Basic Auth | 1 | 1 | Stejná realm, uživatel `admin`, ověřování hesla a 401 challenge |
| klientská IP | 1 | 1 | Použít skutečnou IP z konkrétního transportního klienta pro rate limiter |
| hlavičky | `Origin`, `Host` | `Origin`, `Host` | Odmítnout chybějící, duplicitní či nejednoznačné bezpečnostní hlavičky |
| OTA | upload callback | upload callback | Streamovat multipart tělo; neukládat firmware do parserového bufferu |

## Zásadní technické zjištění

Stávající callbacky jsou svázané s globální instancí `WebServer` a jejími metodami. Autentizace není samostatná: volá `method()`, `hasHeader()`, `header()`, `client().remoteIP()`, `authenticate()` a `requestAuthentication()`. OTA navíc používá `HTTPUpload&` a události `UPLOAD_FILE_START`, `UPLOAD_FILE_WRITE`, `UPLOAD_FILE_END` a `UPLOAD_FILE_ABORTED`.

Parser `HttpRequestParser` není transportní adaptér. Jeho limit těla 1024 B a absence multipart streamu znamenají, že nesmí zpracovávat OTA upload. Předpoklad „přepojíme jen klienta“ je tedy chybný.

## Zvolená implementační strategie

1. **Neudržovat dvě nezávislé kopie rout.** Aplikační obsluha musí být společná a transportní klient musí dodat jen požadavek/odpověď a identitu spojení.
2. **Převést autentizaci na samostatnou službu.** Vstupy: metoda, Origin, Host, klientská IP, Authorization; výstupy: povolit / 401 / 403 / 429 / 503. Rate limiter zůstane společný a per-client.
3. **Převést ne-OTA handlery po skupinách.** Nejdříve pouze GET `/`; následně read-only status a až poté stavové POST cesty. Každý POST zachová stejnou Origin/Host bránu.
4. **OTA oddělit od běžného parseru.** Samostatný multipart stream s limity velikosti, timeoutem, kontrolou odpojení a stejným ověřením přihlášení. Zachovat zákaz OTA při nouzovém režimu; na ESP32 #1 také při aktivním transferu nebo ručním režimu ventilů.
5. **Teprve po paritě všech cest** odstranit závislost na `WebServer`.

## Proč není v REV91 připojen improvizovaný Ethernet server

Přímý nový `EthernetServer` s vlastním dispatchováním by nyní vytvořil druhou implementaci autentizace a callbacků, nebo by obcházel stávající `WebServer`. To by nebyla bezpečná integrace. Neznáme ani přesnou verzi Arduino-ESP32 jádra, takže nelze bez cílového prostředí garantovat kompatibilitu případného zásahu do knihovny `WebServer`.

## Bezpečnostní invariants

- Síťové požadavky nesmějí přímo měnit bezpečnostní relé mimo stávající aplikační logiku.
- Výpadek Wi-Fi, AP nebo W5500 nesmí blokovat ochranné smyčky ani watchdog přepouštění.
- Zachovat fail-closed autentizaci, rate limiting a same-origin POST.
- Zachovat zákaz automatického obnovení přerušeného transferu po restartu.
- Bez komunikace ESP32↔ESP32.
- Arduino cílový build a fyzický test přes W5500 zůstávají povinné; nativní testy je nenahrazují.

## Stav REV91

Jde o technický návrhový krok. Produkční `.ino` soubory, routy, OTA ani síťové chování se nemění. Další bezpečný programový krok je extrakce autentizační služby a transportně nezávislého požadavkového kontextu, nejprve ověřená nativním testem.
