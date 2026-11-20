# HOME WATER CONTROLLER — datagram envelope REV50

Datum: 13. 11. 2026  
Rozsah: přenositelné rozlišení typů UDP datagramů; síťové `.ino` call-sites se v této revizi nemění.

## Proč je obálka nutná

Samotná délka UDP payloadu není bezpečný identifikátor typu. `PeerHandshakeChallenge` má 84 B a autentizovaný status z REV49 má také 84 B. ESP32 #1 přijímá oba typy od ESP32 #2. Přijímač proto nesmí rozhodovat jen podle délky nebo předpokládaného směru.

## Formát

Přidán `PeerDatagramEnvelope` se společnou 8B hlavičkou:

| Offset | Velikost | Význam |
|---:|---:|---|
| 0 | 2 B | Magic `HW` (`0x48 0x57`) |
| 2 | 1 B | Verze obálky, aktuálně `1` |
| 3 | 1 B | Typ: HELLO=1, CHALLENGE=2, FINISH=3, READY=4, STATUS=5 |
| 4 | 2 B | Délka payloadu, big-endian |
| 6 | 2 B | Rezervováno, musí být nula |

Payload zůstává v původním kanonickém formátu jednotlivých modulů. Obálka nepřidává autentizaci: HELLO může být neautentizované, CHALLENGE/FINISH/READY ověřují existující handshake a STATUS musí projít `PeerStatusSessionCodec`. Maximální payload je omezen na 256 B.

## Fail-closed validace

Dekodér odmítne chybný magic, neznámou verzi nebo typ, nenulové rezervované bajty, nulovou či příliš velkou délku a jakýkoli nesoulad deklarované a skutečné délky datagramu. Při chybě vrátí nulový pohled (`payload == nullptr`, délka 0).

## Omezení této revize

- Obálka je implementována a synchronizována v `firmware/common/` a obou sketch `src/`.
- Nativní test ověřuje všechny typy validace a konkrétní kolizi dvou 84B payloadů CHALLENGE/STATUS.
- **Obálka zatím není připojena do UDP smyček `.ino`.** Stávající transport zůstává 52B CRC-only telemetrií.
- Handshake, produkční bezpečné úložiště peer secretu, provisioning, anti-replay integrace a cílový Arduino build zůstávají otevřené.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`; běžný a servisní horní transfer zůstává blokován.

## Testy REV50

- Standardní nativní sada: 69/69 testovacích programů PASS.
- ASan/UBSan sada: 67/67 testovacích programů PASS.
- Společné moduly jsou bajtově shodné v obou sketch `src/`.
- Arduino-ESP32 cílový build, síťový a fyzický test nebyly provedeny.
