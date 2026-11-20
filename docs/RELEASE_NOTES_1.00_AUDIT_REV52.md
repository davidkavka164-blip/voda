# HOME WATER CONTROLLER — release notes REV52

Datum: 13. 11. 2026

## Nový bezpečnostní přijímač autentizovaného statusu

Přidán přenositelný modul `PeerAuthenticatedStatusReceiver` do `firmware/common/` a synchronizován do obou ESP32 sketch `src/`.

Modul centralizuje bezpečné pořadí příjmu stavového rámce:

1. Vyžaduje navázanou a stále platnou instanci `PeerStatusSessionCodec`.
2. Ověří HMAC, směr, identitu peeru a jeho boot ID přes `decodeIncoming()`.
3. Teprve po úspěšném ověření předá rámec do `PeerStatusMonitor`, který kontroluje sekvenci, duplicity a změnu boot epoch.
4. Při chybě vrátí konkrétní důvod a výstupní rámec vynuluje.
5. Po `clearSession()` nebo zneplatnění codec instance odmítá další příjem.

Tím se vytváří jednoznačná testovatelná bezpečnostní hranice mezi UDP parserem a stavem, který může být použit řídicí logikou. Modul zatím **není volán z `.ino` UDP smyček**.

## Testy

- Standardní nativní sada: 70/70 testovacích programů PASS.
- ASan/UBSan sada: 68/68 testovacích programů PASS.
- Nový test ověřuje odmítnutí bez session, správné přijetí HMAC statusu, odmítnutí duplicitní sekvence, odmítnutí pozměněného rámce a to, že neautentizovaný rámec nemění obsah ani čas posledního přijetí v monitoru.
- Kopie nového modulu v `firmware/common/`, `firmware/ESP32_1/src/` a `firmware/ESP32_2/src/` jsou bajtově shodné.
- Arduino-ESP32 cílový build, skutečný UDP handshake, NVS Encryption, fyzický provisioning a hardware testy nebyly provedeny.

## Bezpečnostní stav zůstává beze změny

- Produkční UDP transport stále používá 52B CRC-only status.
- Produkční peer-secret backend s prokázaným důvěrným uložením není implementován.
- Handshake není zapojen do `.ino`.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Běžný automatický a servisní transfer horní → dolní zůstává blokován.
- Nouzové ruční otevření zůstává samostatné, výslovně potvrzené a pod dohledem obsluhy.

REV52 je dílčí implementační revize přenositelné přijímací hranice, nikoli dokončení autentizované komunikace.
