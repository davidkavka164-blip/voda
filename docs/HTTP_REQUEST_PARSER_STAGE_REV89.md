# HWC — transportně nezávislý HTTP parser, etapa REV89

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV88

## Účel této etapy

Připravit první samostatně testovatelnou část budoucího adaptéru Wi-Fi + W5500: parser jednoho HTTP/1.0 nebo HTTP/1.1 požadavku bez závislosti na `WiFiClient`, `EthernetClient` nebo Arduino `WebServer`.

Parser je v `firmware/common/HttpRequestParser.h` a jeho identické kopie jsou v `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.

## Co parser nyní umí

- Rozpozná GET a POST a origin-form cestu začínající `/`.
- Rozpozná HTTP/1.0 a HTTP/1.1.
- Načte omezený počet hlaviček, kontroluje syntaxi a omezuje jejich celkovou velikost.
- Zpracuje query string a `application/x-www-form-urlencoded` tělo POST.
- Dekóduje `%HH` a `+`, zachovává opakované parametry a jejich pořadí.
- Odmítne neplatné escapování, řídicí znaky, duplicitní/nejasné bezpečnostní hlavičky při jejich vyhledávání, nejednoznačné Content-Length, `Transfer-Encoding`, příliš velké či neúplné tělo a HTTP pipelining.
- Používá pevně omezená pole a sdílený zásobník dekódovaných hodnot; nealokuje dynamické řetězce. Request kontext má na hostitelském ABI při kontrole 6120 B; na ESP32 se přesná velikost může mírně lišit. Při pozdější integraci musí být staticky nebo jinak dlouhodobě alokován, nikoli jako velká lokální proměnná v síťové callback funkci.

## Limity

| Oblast | Limit |
|---|---:|
| Request line | 191 B |
| Cesta | 127 B |
| Query | 255 B |
| Všechny hlavičky | 1024 B |
| Počet hlaviček | 16 |
| Název hlavičky | 31 B |
| Hodnota hlavičky | 127 B |
| Běžné tělo POST | 1024 B |
| Počet parametrů | 24 |
| Název parametru | 31 B |
| Jednotlivá hodnota parametru | 511 B |
| Součet dekódovaných hodnot | 1024 B |

## Záměrné hranice

Toto není síťový server ani kompletní náhrada `WebServer`. Parser nečte socket, neřeší timeouty, autentizaci, rate limiter, Origin/Host politiku, odpovědi, routování ani OTA. Multipart a raw binary upload jsou výslovně mimo tento parser a musí mít samostatnou streamovanou cestu s událostmi START/WRITE/END/ABORTED.

Zatím není zapojen do produkčních `.ino`. Stávající webserver, routy, přihlášení, CSRF ochrana, OTA a bezpečnostní smyčky zůstávají beze změny.

## Hostitelské testy

`tests/test_http_request_parser.cpp` pokrývá validní GET/POST, query a formuláře, URL decoding, opakované parametry, chybné metody/cesty/verze, neúplné řádky a těla, chybné procentové kódování, duplicitní hlavičky, chunked transfer a limity velikosti.

Test se spouští přes `tests/run_native_tests.sh` pod C++11 s `-Wall -Wextra -Werror -pedantic`.

## Další etapa

1. Přidat bezpečný transportní reader s pevnými timeouty a limity, který parseru předá přesně jeden kompletní požadavek.
2. Vytvořit kompatibilní request/response facade pro současné handlery, aby nebyly kopírovány routy.
3. Převést autentizaci, rate limiter a Origin/Host kontrolu na společný request kontext.
4. OTA řešit samostatně jako stream; nepoužít limit běžného těla 1024 B.
5. Až poté připojit Wi-Fi a Ethernet klienty a testovat paritu všech 28 rout.

Cílový Arduino build a fyzické testy nebyly v této etapě provedeny.
