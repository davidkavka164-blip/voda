# HOME WATER CONTROLLER DEV KIT 0.89

Datum: 2026-11-04

## Rozšíření regresních testů fyzické servisní autorizace

- Rozšířen `test_local_service_authorization.cpp` o test podržení kratšího než 3 s a přesného dosažení limitu.
- Doplněn test, že nepřetržitě aktivní vstup nevytvoří druhou autorizaci po spotřebování první autorizace ani po uplynutí jejího okna.
- Doplněn test, že uvolnění a nový stisk zahájí nové měření doby podržení; předchozí čas se nepřenáší.
- Doplněna kontrola expirace přesně na hranici 60 s při přetečení `millis()`.
- Samotná politika `LocalServiceAuthorization` se funkčně neměnila; hlavička zůstává synchronizována ve společné složce a ve zdrojových stromech obou ESP32.

## Výsledky ověření

- Standardní nativní testy: 48/48 programů PASS.
- ASan/UBSan: 48/48 programů PASS.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- Arduino-ESP32 build, fyzický servisní vstup, provisioning, skutečná NVS a NVS/Flash Encryption nebyly ověřeny.
- Servisní autorizační modul není připojen do `.ino` ani k GPIO. CredentialStore není zapojen do AP/web boot call-sites. Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.
