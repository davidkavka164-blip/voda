# HOME WATER CONTROLLER — MASTER 3.11 / REV88

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV87

## REV88 — přesný kontrakt HTTP adaptéru pro Wi-Fi + W5500

- Zmapováno všech 28 současných HTTP rout: ESP32 #1 má 17 rout (1 GET, 16 POST), ESP32 #2 má 11 rout (1 GET, 10 POST).
- Statická kontrola ověřila, že každý route callback přímo volá svou autentizační bránu; obě OTA routy mají samostatný upload callback a obě jednotky autentizovanou větev `onNotFound`.
- Popsáno úplné rozhraní současného `WebServer`, na kterém závisí aplikační handlery: routování, request parametry a hlavičky, klientská IP, Basic Auth, odpovědi a multipart OTA.
- Identifikována nutnost odstranit typovou vazbu `isSameOriginPost(WebServer&)` před vložením alternativního transportu. Adaptér musí zachovat stávající callbacky a bezpečnostní logiku, nikoli vytvořit paralelní kopie rout.
- Přidán opakovatelný nástroj `tools/audit_http_server_surface.py` a kontrakt `docs/HTTP_SERVER_ADAPTER_CONTRACT_REV88.md`.
- Produkční `.ino` soubory se nemění. Ethernetové HTTP ovládání zatím není implementováno.
- Nativní a statické kontroly ověřují pouze existující kód a inventář rout. Neprokazují běh HTTP adaptéru ani OTA přes W5500.
- Arduino cílový build a fyzické testy nebyly provedeny. Přesná instalovaná verze Arduino-ESP32 core zůstává nepotvrzená.

## Neměnné bezpečnostní zásady

- Každá stavová akce zůstává chráněna autentizací, rate limiterem a Origin/Host kontrolou POST.
- OTA zůstává streamovaná a musí být přerušena při odpojení/chybě; úspěch lze potvrdit až po ověřeném dokončení zápisu.
- Síťová obsluha nesmí blokovat ochranné smyčky ani měnit výchozí bezpečné stavy relé.
- Wi-Fi STA/AP a W5500 mají být nezávislé transporty téhož aplikačního rozhraní.
- Mezi ESP32 se neobnovuje žádná peer komunikace.
