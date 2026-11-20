# HOME WATER CONTROLLER — vývojový balíček 1.00 REV50

Datum: 13. 11. 2026

## Jednoznačné označení peer UDP datagramů

- Přidán `PeerDatagramEnvelope`: 8B hlavička s magic, verzí, typem, délkou payloadu a rezervovanými nulovými bajty.
- Typy HELLO, CHALLENGE, FINISH, READY a STATUS jsou rozlišeny nezávisle na délce payloadu.
- Řeší konkrétní kolizi: handshake CHALLENGE i autentizovaný STATUS mají 84B payload.
- Dekodér fail-closed odmítá neznámé typy/verze, chybnou délku, poškozený magic a nenulové rezervované bajty.
- Modul a test jsou synchronizovány ve společné složce a obou ESP32 sketchech.

## Co zůstává otevřené

- Obálka zatím není napojena do `.ino` UDP smyček; stávající transport zůstává 52B CRC-only.
- Handshake HELLO/CHALLENGE/FINISH/READY není napojen do UDP socketů.
- Produkční důvěrné úložiště peer secretu a fyzický provisioning nejsou dokončeny.
- Anti-replay musí být aplikován po autentizaci a před publikací dat do `PeerStatusMonitor`.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`; běžný a servisní horní transfer zůstává blokován.

## Testy

- Standardní nativní sada: 69/69 PASS.
- ASan/UBSan: 67/67 PASS.
- Synchronizace společných modulů: PASS.
- Arduino cílový build a fyzické testy: neprovedeny.

Podrobnosti: `docs/PEER_DATAGRAM_ENVELOPE_REV50.md`.
