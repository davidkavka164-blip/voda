# HOME WATER CONTROLLER — vývojový balíček 1.00 REV49

Datum: 13. 11. 2026

## Autentizovaný status codec

- Přidán `PeerStatusSessionCodec` pro původní 52B status rámec. Autentizovaný rámec má 84 B (52B payload + 32B HMAC-SHA-256).
- Codec používá existující směrové session klíče, kontroluje device ID a obě boot epoch, ověřuje HMAC před dekódováním a ponechává CRC jako kontrolu náhodného poškození.
- Modul je synchronizován v `firmware/common/` a obou sketch `src/`.
- Nový test pokrývá validní přenos, změnu payloadu/tagu, zkrácení, chybný boot ID, směr i device ID.

## Co zůstává otevřené

- Codec zatím není připojen do UDP smyček `.ino`; transport stále používá 52B CRC-only status.
- Handshake HELLO/CHALLENGE/FINISH/READY není připojen do skutečných UDP socketů.
- Produkční bezpečné úložiště peer secretu a fyzický provisioning nejsou dokončeny.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`; běžný a servisní transfer horní cestou zůstává blokován.
- Anti-replay a freshness se musí po autentizaci uplatnit před publikací dat do `PeerStatusMonitor`.

## Testy

- Standardní nativní sada: 68/68 PASS.
- ASan/UBSan: 66/66 PASS.
- Arduino cílový build a fyzické testy: neprovedeny.

Podrobnosti: `docs/PEER_STATUS_AUTH_CODEC_REV49.md` a předchozí `docs/PEER_AUTH_TRANSPORT_INTEGRATION_AUDIT_REV48.md`.
