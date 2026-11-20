# HWC HTTP transport integration gate — REV90

Datum: 19. 11. 2026  
Východisko: skutečný archiv DEV KIT 1.00 REV89.

## Zjištěný stav zdrojového kódu

Obě jednotky stále používají Arduino `WebServer`:

- ESP32 #1: `WebServer webServer(80)`, 17 registrovaných cest.
- ESP32 #2: `WebServer localWebServer(80)`, 11 registrovaných cest.
- Celkem: 28 cest. Stávající smyčky volají `handleClient()` na `WebServer` instanci.
- W5500 je inicializován přes `Ethernet`/DHCP, ale samostatný `EthernetServer` ani HTTP obsluha přes Ethernet v produkčních sketchech nejsou.

## Proč zatím nelze připojit parser přímo

Produkční handlery nejsou napsané proti neutrálnímu HTTP rozhraní. Přímo používají `WebServer` API: `on`, `onNotFound`, `collectHeaders`, `method`, `hasArg`, `arg`, `send`, `client`, `authenticate`, `requestAuthentication`, `upload`, `begin` a `handleClient`. Autentizační pomocné funkce navíc přijímají `WebServer&` a čtou `Origin`, `Host` a IP klienta.

Parser z REV89 je pouze syntaktická vrstva. Má limit těla 1024 B, podporuje jen GET/POST a URL-encoded formuláře. Záměrně neobsluhuje multipart OTA. Nelze jej proto bez další práce považovat za náhradu `WebServer` ani jej zapojit do existujících callbacků.

## Bezpečný směr integrace

1. Zavést transportní rozhraní, které reprezentuje klienta, hlavičky, parametry, odpověď, IP adresu a upload stream.
2. Vytvořit kompatibilní vrstvu pro stávající callbacky; nejdříve přenést jen GET `/` a ověřit shodné chování na Wi-Fi a Ethernetu.
3. Přesunout autentizaci do transportně nezávislé služby. Zachovat Basic Auth, rate limiter a přesné ověření Origin/Host; chybějící nebo nejednoznačné hlavičky musí znamenat odmítnutí POST.
4. Přenášet po jedné skupině POST cest a porovnávat odpovědi i stavové účinky s původním serverem.
5. OTA řešit odděleným streamovacím adaptérem s limity, timeouty, kontrolou odpojení a stejnou autentizační bránou. Neposílat firmware přes pevný 1kB parser.
6. Teprve po úplném pokrytí cest vypnout původní `WebServer` závislost.

## Bezpečnostní a provozní brány

- Wi-Fi STA, konfigurační AP a W5500 jsou nezávislé transporty; výpadek jednoho nesmí ovlivnit druhý.
- Žádná síťová funkce nesmí blokovat bezpečnostní smyčky čerpadel, ventilů ani watchdog přepouštění.
- Nepřidávat komunikaci ESP32↔ESP32.
- Zachovat bezpečné výchozí vypnutí relé při startu a při interní chybě.
- Neznámé cesty, chybná délka těla, duplicitní bezpečnostní hlavičky a neověřitelné POST odmítat.
- Cílový Arduino build a fyzický test Ethernetu jsou samostatné povinné brány; hostitelský test je nenahrazuje.

## Rozsah REV90

REV90 nepřidává Ethernetové HTTP ovládání a nemění produkční firmware. Přidává automatickou statickou kontrolu integračního kontraktu, aby se při dalším refaktoringu neztratila žádná z 28 cest, autentizační API ani OTA upload callback. Je to záměrná bezpečnostní brána před implementací adaptéru, nikoli hotová Ethernetová funkce.
