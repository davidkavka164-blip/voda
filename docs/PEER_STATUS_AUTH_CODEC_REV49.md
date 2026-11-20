# HOME WATER CONTROLLER — autentizovaný status codec REV49

Datum: 2026-11-13  
Rozsah: přenositelná serializační a autentizační vrstva pro existující 52B status ESP32 #2 → ESP32 #1. UDP `.ino` call-sites se v této revizi nemění.

## 1. Implementace

Přidán `PeerStatusSessionCodec`:

- Zachovává původní kanonický 52B `PeerStatusFrame` a jeho CRC pro detekci náhodného poškození.
- Za původní payload přidává plný 32B HMAC-SHA-256 tag; autentizovaný datagram má 84 B.
- HMAC pokrývá přesně všech 52 B payloadu včetně CRC. Výpočet používá již existující směrové session klíče z `derivePeerDirectionalKeys()`.
- Responder (ESP32 #2) pro odeslání používá `responderToInitiator`; initiator (ESP32 #1) pro příjem ověřuje stejný klíč. Opačný směr používá oddělený klíč.
- Kontext codec je připnut na lokální/peer device ID a obě boot ID. Odeslání odmítne rámec s cizí lokální identitou/boot ID; příjem po ověření HMAC odmítne cizí peer ID/boot ID.
- Dekódování nejprve ověří konstantně časový HMAC tag, až potom dekóduje a validuje CRC/status. Při chybě vynuluje výstupní rámec.
- Codec nepřijímá ani neukládá dlouhodobý secret a nevytváří session. Lze jej vytvořit až po úspěšném handshake/READY a derivaci směrových klíčů.

Soubory jsou bajtově synchronizovány do `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/`.

## 2. Testy

Nový `test_peer_status_session_codec.cpp` ověřuje:

- úspěšné ověření statusu responder → initiator;
- zachování všech datových polí;
- odmítnutí změny payloadu i tagu a zkráceného rámce;
- odmítnutí nesprávného peer boot ID a nesprávné role/směru;
- odmítnutí rámce s cizí device ID nebo lokálním boot ID;
- vynulování výstupu při neúspěšném dekódování.

Test je přidán do standardní i ASan/UBSan sady.

## 3. Bezpečnostní hranice

Tato revize **nezapojuje codec do UDP**. ESP32 #1 stále přijímá 52B CRC-only rámec a ESP32 #2 jej stále vysílá. `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`; horní automatický i servisní transfer zůstává blokován.

Codec sám neřeší anti-replay. Po ověření tagu musí volající předat rámec do `PeerStatusMonitor` (sekvence/boot epoch/freshness) a při změně session nebo peer boot epoch invalidovat dosavadní stav. HMAC nenahrazuje bezpečné uložení dlouhodobého peer secretu ani fyzický provisioning.

## 4. Výsledky

- Standardní nativní testovací programy: 68/68 PASS.
- ASan/UBSan testovací programy: 66/66 PASS.
- Statická kontrola shody `firmware/common` s oběma sketch `src`: PASS.
- Arduino-ESP32 cílový překlad: neproveden.
- UDP integrace a fyzický test: neprovedeny.
