# HOME WATER CONTROLLER — MASTER 3.13 / REV90

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV88

## REV89 — první transportně nezávislá část HTTP adaptéru

- Přidán pevně omezený C++11 parser jednoho HTTP/1.x požadavku do `firmware/common/HttpRequestParser.h`.
- Parser zpracovává GET/POST, cestu, hlavičky, query parametry a URL-encoded POST formuláře. Dekódované hodnoty sdílí pevně omezený buffer, aby nebyla každá hodnota alokována v samostatném velkém poli.
- Záměrně odmítá chunked transfer, nepodporované typy těla, chybný Content-Length, neúplné požadavky, přetečení limitů a HTTP pipelining.
- Multipart/raw OTA upload zůstává mimo parser a musí být řešen samostatným streamem.
- Přidán hostitelský test `tests/test_http_request_parser.cpp` do hlavní sady nativních testů.
- Společný modul je synchronizován do obou Arduino sketch `src/` adresářů.
- Produkční `.ino` se nemění; parser zatím není zapojen do obsluhy HTTP.
- Autentizace, rate limiter, Origin/Host ochrana, OTA, síťové nastavení, W5500, GPIO, NVS, čerpadla a ventily zůstávají beze změny.
- Hostitelské testy ověřují parser, nikoli síťový běh, cílový Arduino build ani fyzické zařízení.

## Neměnné bezpečnostní zásady

- Všechny stavové HTTP akce zůstávají chráněny autentizací, rate limiterem a kontrolou Origin/Host.
- Neznámé cesty musí projít stejnou autentizační bránou jako dosud.
- OTA musí být streamovaná a musí být přerušena při odpojení nebo chybě.
- Síťová obsluha nesmí blokovat ochranné smyčky čerpadel a ventilů.
- Wi-Fi STA/AP a W5500 mají být nezávislé transporty stejného aplikačního rozhraní.
- Mezi ESP32 se neobnovuje peer komunikace.

## REV90 — integrační kontrakt transportní vrstvy

- Ověřen skutečný stav obou produkčních sketch: ESP32 #1 má 17 a ESP32 #2 má 11 rout; dohromady 28.
- Zjištěno, že produkční routy jsou přímo svázané s Arduino `WebServer` API a že autorizační pomocné funkce přijímají `WebServer&`.
- Potvrzeno, že W5500 má síťovou/DHCP inicializaci, ale v produkčních sketchech není `EthernetServer` ani HTTP obsluha přes Ethernet.
- Parser REV89 je omezen na 1024 B těla, GET/POST a URL-encoded formuláře; multipart OTA zůstává oddělené.
- Přidán `tools/audit_http_transport_contract.py`, který hlídá všech 28 rout, auth/header API a OTA upload callback.
- Přidán `docs/HTTP_TRANSPORT_INTEGRATION_GATE_REV90.md` s bezpečným pořadím refaktoringu.
- Produkční firmware se v REV90 nemění. Žádná nová síťová funkce není deklarována jako hotová.
- Nebyl proveden Arduino target build ani fyzický Ethernet test.
