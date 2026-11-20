# W5500 – webové rozhraní ESP32 #2 (REV69 audit)

## Aktuální implementace
W5500 zajišťuje DHCP/IP konektivitu. `WebServer localWebServer(80)` však zůstává obsluhován servisním Wi-Fi AP; Ethernet klienti nejsou tímto serverem obslouženi. Nelze tedy tvrdit, že web ESP32 #2 je dostupný přes LAN/W5500.

## Zjištění kompatibility
- `WebServer_ESP32_W5500` uvádí pro ESP32_DEV s W5500 požadavek ESP32 core 2.0.6 nebo novější. To není bezpečná cesta pro uživatelovo současné Windows 7 SP1, kde je core 2.x problematický/nepodporovaný.
- SparkFun WebServer ESP32 W5500 popisuje přímé použití nízkoúrovňových ESP-IDF síťových/SPI/interrupt rozhraní a upozorňuje na konflikty, pokud další části aplikace také nastavují SPI nebo přerušení. Projekt již používá standardní `Ethernet.h`/SPI pro DHCP, takže souběžné zavedení této knihovny by bez cílené architektonické změny představovalo riziko.
- Dvě oddělené instance serveru se stejnými routami by vyžadovaly duplicitní registraci a ošetření autentizace, uploadu OTA a klientského kontextu. Nesmí se pouze přidat další server bez ověření, že knihovna používá kompatibilní transport a že všechny POST/OTA cesty fungují stejně.

## Rozhodnutí REV69
W5500 web server zatím neimplementovat. Zachovat současný servisní AP a DHCP rozhraní bez změn. Před další implementací zjistit přesnou verzi Arduino-ESP32 core a knihovny Ethernet v Arduino IDE 1.8.19. Pokud zůstane core 1.0.6, vybrat řešení kompatibilní s tímto core nebo změnit platformu/IDE na podporovaném počítači; žádnou novou síťovou knihovnu nepřidávat naslepo.

## Bezpečnostní požadavky pro budoucí implementaci
- Zachovat servisní AP jako nezávislou cestu i při ztrátě Ethernetu/DHCP.
- Zachovat autentizaci, CSRF/origin kontrolu, rate limiting a omezení OTA.
- HTTP správa pouze v důvěryhodné lokální síti nebo přes VPN; neprovádět port-forward veřejného HTTP.
- Ověřit současné požadavky přes AP i Ethernet, včetně loginu, všech POST formulářů, OTA uploadu, výpadku kabelu, DHCP obnovy a restartu.

## Odkazy ověřené při auditu
- WebServer_ESP32_W5500 (požadavek ESP32 core 2.0.6+): https://github.com/khoih-prog/WebServer_ESP32_W5500
- SparkFun WebServer ESP32 W5500 (poznámky ke konfliktům SPI/interruptů): https://github.com/sparkfun/SparkFun_WebServer_ESP32_W5500
