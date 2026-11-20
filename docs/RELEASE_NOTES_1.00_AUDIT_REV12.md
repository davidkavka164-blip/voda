# DEV KIT 1.00 AUDIT — REV12

Datum: 2026-11-08

## Změny
- Přidán `NVS_PEER_REBOOT_WEB_AUDIT_REV11.md`.
- Zmapována odolnost NVS úložišť a hranice dosavadního hostitelského ověření.
- Popsáno zapojení/nezapojení peer handshake a bezpečnostní limity UDP statusu.
- Prověřeno chování lokální ochrany ESP32 #2 po restartu a detekována mezera při rychlé změně peer boot epoch během aktivního transferu.
- Porovnáno hlavní webové rozhraní s MASTER 1.3; potvrzeno, že nouzové ruční ovládání ventilů není implementováno.
- Zdokumentováno, že existující `upperValveDecision` a hystereze nejsou zapojeny do `TransferLogic` prechecku.

## Změny firmwaru
Žádné. Limity hladin, hystereze, pump protection a transferová logika nebyly touto revizí změněny.

## Ověření
- Dokumentační/static audit REV11 zdrojů.
- Neproveden Arduino-ESP32 build ani fyzický test ESP32/NVS/W5500/ventilů.
