# HOME WATER CONTROLLER 0.26

## Nové
- ESP32 #2 má na svém místním autentizovaném AP rozhraní upload firmwaru `.bin` přes Arduino `Update`.
- Ošetřena autorizace uploadu, zahájení zápisu, částečný zápis, přerušení, výsledek `Update.end(true)` a potvrzovací stránka před restartem.
- Diagnostika shody konfigurace z 0.25 zůstává zachována: peer protokol v2/52 B a blokace simulace horního transferu při rozdílných limitech.

## Bezpečnostní omezení
- OTA je dostupné pouze na místním AP s Basic Auth (bez TLS); přístupový bod nesmí být mostem do nedůvěryhodné sítě.
- Neověřeno: partition tabulka, skutečná velikost firmwaru, správný binární soubor pro ESP32 #2, přerušení napájení během zápisu, OTA rollback a fyzické zařízení.
- Výkonové výstupy zůstávají neaktivní.

## Ověření
- Výsledky nativních testů, hostitelské syntax kontroly, synchronizace a integrity ZIP jsou v `TEST_AND_REVIEW_LOG.md`.
- Hostitelská syntaktická kontrola není náhradou Arduino-ESP32 buildu.
