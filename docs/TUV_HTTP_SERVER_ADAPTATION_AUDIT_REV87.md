# HOME WATER CONTROLLER — vyhodnocení TUV Ethernet webserveru a návrh bezpečné integrace REV87

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV86  
Nový referenční zdroj: uživatelem dodaný archiv `TUV_Controller_3_senzor2.1_newWifi_WIFI_RECONNECT_AP_WEB_FIX(1).zip`

## 1. Co TUV projekt prokazuje

TUV projekt obsahuje funkční koncepci dvou TCP serverů na portu 80:

- `WiFiServer server(80)` pro Wi-Fi,
- `EthernetServer ethernetServer(80)` pro W5500 přes Arduino `Ethernet.h`,
- společnou šablonovou funkci `processWebClient(ClientType &client)`, která přijímá `WiFiClient` i `EthernetClient`,
- společné odesílání odpovědí přes `client.write()`.

Tím je potvrzeno, že v rámci použitého Arduino Ethernet stacku lze přijímat HTTP spojení přes W5500 a obsloužit jej stejným transportně obecným kódem. Není to však hotový adaptér pro HWC `WebServer`.

## 2. Proč nelze TUV `processWebClient()` převzít přímo

TUV HTTP obsluha si sama parsuje první řádek a hlavičky. Většina běžných akcí je v TUV implementována jako GET s parametry v URL. Zkontrolovaný kód:

- neimplementuje obecný parser těla formuláře `application/x-www-form-urlencoded` pro POST;
- neprovádí Origin/Host ochranu stavových požadavků;
- nevyžaduje Basic Auth pro běžné stránky a ovládací cesty (ověření je v ukázce soustředěno na OTA);
- používá jednodušší vlastní OTA tok s pevným Content-Length.

HWC naproti tomu používá `WebServer` callbacky, POST formuláře, Basic Auth, rate limiter, povinnou kontrolu `Origin` + `Host` pro každý POST a upload OTA přes `HTTPUpload` callback. Přímé převzetí TUV parseru by tedy změnilo bezpečnostní model a mohlo by zpřístupnit nebo chybně zpracovat ovládací akce. Takový port se nesmí provést.

## 3. Zjištěný rozsah stávajícího HWC webu

### ESP32 #1

WebServer registruje:

- `GET /`
- `POST /geometry`
- `POST /calibration`
- `POST /sensor-calibration`
- `POST /sensor-recovery`
- `POST /protection-config`
- `POST /lower-pump-protection-config`
- `POST /lower-pump-emergency`
- `POST /transfer-start`
- `POST /transfer-stop`
- `POST /inflow-watchdog-config`
- `POST /transfer-recovery-ack`
- `POST /pump-emergency`
- `POST /manual-valve-mode`
- `POST /manual-valve-command`
- `POST /wifi`
- `POST /update` s odděleným upload callbackem
- autentizovaná větev `onNotFound`.

Všechny stavové handlery používají `requireWebAuth()`. Funkce ověřuje Origin/Host u POST, dostupnost administrátorského hesla, rate limiter a Basic Auth. OTA upload má vlastní průběžné ověřování a abort při chybě.

### ESP32 #2

WebServer registruje:

- `GET /`
- `POST /local-limits`
- `POST /volume-anchor`
- `POST /volume-corrupt-reset`
- `POST /volume-point`
- `POST /volume-reference`
- `POST /volume-reference-cancel`
- `POST /volume-transfer`
- `POST /sensor-recovery`
- `POST /pump-emergency`
- `POST /update` s odděleným upload callbackem
- autentizovanou větev `onNotFound`.

Všechny stavové handlery používají `requireLocalAuth()`, které obdobně kontroluje Origin/Host, dostupnost přihlašovacích údajů, rate limiter a Basic Auth. Servisní SoftAP musí zůstat dostupný i při současném Ethernetu.

## 4. Bezpečný cílový návrh

Cílem zůstává stejné plnohodnotné webové rozhraní přes Wi-Fi/AP i W5500. Implementace musí mít společnou aplikační logiku a dva transportní vstupy. Doporučené vrstvy:

1. **HTTP transport** – přijímá `WiFiClient` nebo `EthernetClient`, nastavuje limity a timeouty, čte request line, hlavičky a tělo.
2. **Normalizovaný požadavek** – metoda, cesta, query, hlavičky, délka a bezpečně načtené tělo formuláře; remote IP zachováno pro rate limiter.
3. **Bezpečnostní brána** – jedna společná kontrola Basic Auth, rate limiteru a Origin/Host pro POST; nesmí existovat cesta k handleru, která ji obejde.
4. **Aplikační handlery** – sdílená logika HWC bez přímé závislosti na globální instanci `WebServer`; stejná validace a stejné výsledky pro oba transporty.
5. **OTA adaptér** – streaming po blocích, ověření oprávnění před zahájením, kontrola délky a výsledku zápisu, abort při odpojení/chybě, potvrzení až po úspěšném `Update.end(true)`.

Pouhé vytvoření druhého `EthernetServer` vedle současného `WebServer` nestačí, protože současné callbacky čtou argumenty, hlavičky, klientskou IP a OTA upload přímo z globální instance `WebServer`. Je nutné tyto závislosti nejprve oddělit od aplikační logiky, nebo napsat plnohodnotný kompatibilní adaptér. Zvolená implementace musí být ověřena na přesné verzi Arduino-ESP32 core; nelze předpokládat, že se API `WebServer`, `WiFiServer` a `EthernetServer` mezi core verzemi chová stejně.

## 5. Síťové a bezpečnostní invarianty

- W5500 DHCP/link recovery nesmí blokovat hlavní smyčku déle, než stanoví bezpečné limity, a nesmí měnit výstupy pump, ventilů ani transferový stav.
- Výpadek Ethernetu nesmí vypnout Wi-Fi STA/AP; výpadek Wi-Fi nesmí vypnout Ethernet.
- ESP32 #2 si ponechá lokální servisní AP i při Ethernetu.
- Oba transporty poskytují stejné routy, autentizaci, autorizaci, ochranu proti CSRF a chování OTA.
- Vzdálená správa z internetu bude řešena VPN; nepřidávat veřejné přesměrování nešifrovaného HTTP.
- Bez peer komunikace mezi ESP32; síťové rozhraní každé jednotky slouží jejímu vlastnímu přístupu.
- Bez změny GPIO, relé, čidel, NVS schémat, čerpadlových ochran, ventilů, transferového watchdogu a nouzových režimů.

## 6. Testovací brána před označením implementace za hotovou

1. Porovnat seznam rout Wi-Fi a Ethernet pro každou jednotku.
2. Ověřit chybné/nezadané heslo, rate limiting a platné přihlášení na obou transportech.
3. Ověřit POST bez Origin, bez Host, s cizím Origin a se správným Origin.
4. Ověřit všechny formuláře, včetně chybného a hraničního vstupu.
5. Ověřit OTA: přerušení kabelu, neúplný soubor, chybný obsah, chyba zápisu a úspěšné dokončení.
6. Ověřit souběžný Wi-Fi a Ethernet klient, opakované připojení, změnu DHCP adresy a odpojení/připojení kabelu.
7. Během všech síťových testů ověřit, že lokální ochranné smyčky a výchozí bezpečné stavy relé pracují nezávisle.
8. Pro každý cílový ESP32 core provést Arduino target build a následně fyzické testy. Nativní testy ani statická kontrola nenahrazují target build či test na zařízení.

## 7. Stav REV87

Tato revize je **návrhový a integrační audit**, nikoli implementace Ethernet webu. Zdrojový firmware ESP32 #1 a ESP32 #2 zůstává beze změny. TUV archiv byl využit jako potvrzení použitelnosti společného HTTP transportního handleru, nikoli jako zdroj bezpečnostní logiky HWC.

Při kontrole základu REV86 po vytvoření auditu prošlo `tests/run_native_tests.sh`: **79 nativních testovacích programů** a navazující statické integrační kontroly včetně ověření nezávislých síťových cest, autentizace, Origin/Host ochrany a OTA bezpečnostních blokací. Tyto výsledky ověřují existující REV86 logiku; protože firmware nebyl změněn, nejde o test nové Ethernet HTTP implementace. Arduino target build ani fyzický test W5500/HTTP nebyl proveden.

Další implementační krok je rozdělit HTTP aplikační logiku HWC od `WebServer` API při zachování stávajících handlerů a jejich bezpečnostních kontrol. Teprve potom přidat Ethernet transport. Přesná instalovaná verze Arduino-ESP32 core zůstává neověřená; cílový build proto zatím nelze prohlásit za ověřený.
