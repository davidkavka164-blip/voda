# HOME WATER CONTROLLER — jednoduché HMAC základy REV58

Datum: 14. 11. 2026  
Vychází z: `HOME_WATER_CONTROLLER_DEV_KIT_1.00_AUDIT_DRAFT_REV57.zip`  
Rozsah: samostatné přenositelné moduly a host testy; bez zapojení do produkčních UDP smyček.

## 1. Co bylo doplněno

### `PeerStatusHmacCodec`

- Zachovává původní 52B status rámec.
- Za rámec připojuje plný 32B HMAC-SHA256; výsledný UDP payload bude 84 B.
- Používá přímo jeden společný 32B klíč; žádné session keys, handshake ani výměna klíčů.
- HMAC ověřuje přesných 52 B před dekódováním CRC rámce.
- Kontrola tagu používá konstantně-časové porovnání.
- Zachovává původní `bootId`, `sequence`, CRC a validaci identifikátoru zařízení.

### `NvsPeerSharedKeyStore`

- Minimální úložiště jediného 32B klíče v Preferences/NVS namespace `hwc_peer`, položka `shared`.
- Při načítání kontroluje přesnou délku a odmítá prázdný/nulový klíč.
- Při zápisu provede read-back a porovná všech 32 B.
- Při chybě ověření odstraní neověřený záznam.
- Úložiště je záměrně obyčejné NVS, nikoli šifrovaný trezor. Fyzické přečtení flash může klíč odhalit.

Oba moduly jsou v `firmware/common` a jsou byte-identické v obou složkách `firmware/ESP32_1/src` a `firmware/ESP32_2/src`.

## 2. Co zůstává nezměněno

- Produkční UDP smyčky ESP32 #1 a ESP32 #2 stále posílají/přijímají původní 52B CRC rámec.
- Nový HMAC codec ani NVS key store zatím nejsou volány z `.ino`.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- CRC-only telemetrie tedy nemůže povolit běžný ani servisní horní transfer.
- Nouzový ruční ventilový režim zůstává oddělen podle předchozích pravidel.
- Nebyly měněny GPIO, pump safety relays, transfer logika, síťová topologie, NVS partition ani eFuse.

## 3. Testy v této revizi

- `tests/run_native_tests.sh`: 72 nativních testovacích programů PASS; statické integrační kontroly PASS.
- `tests/.run_native_tests_asan.sh`: 70 testovacích programů s ASan/UBSan PASS; statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- ZIP integrity: ověřit při zabalení release.

Nové testy ověřují správný/nesprávný klíč, změněný payload, chybnou délku, nesprávné ID zařízení, odmítnutí nulového klíče a ukládání/načítání klíče s kontrolou read-back.

## 4. Co ještě není hotovo

1. Jednoduchý fyzický způsob vložení stejného klíče do obou ESP32. Stávající provisioning AP/admin hesel není pro tento účel automaticky vhodný a nesmí se bez auditu rozšiřovat.
2. Rozhodnout, zda bude klíč vkládán jednorázově přes USB Serial (bez výpisu klíče do logu), nebo jiným jednoduchým fyzicky potvrzeným postupem.
3. Zapojit načtení klíče a HMAC codec do ESP32 #2 senderu a ESP32 #1 receiveru. Přijímač musí nejdříve ověřit HMAC, teprve potom předat rámec do `PeerStatusMonitor`.
4. Ověřit Arduino cílový překlad v přesné uživatelově verzi core a skutečnou UDP komunikaci na deskách.
5. Teprve po samostatném ověření posoudit povolení horního transferu. Nyní se brána nesmí zapnout.

## 5. Omezení anti-replay

Stávající `PeerStatusMonitor` odmítá duplicitní a starší sekvence a hlídá několik nedávných boot epoch v RAM. Historie se po restartu ESP32 #1 ztratí. REV58 nepřidává trvalý replay journal ani handshake. To je vědomý kompromis této jednoduché varianty a musí zůstat zdokumentován.

## 6. Výslovné hranice

Nativní testy ověřují přenositelné C++ moduly, nikoli Arduino cílový překlad, skutečný NVS čip, ethernetový UDP provoz ani fyzické relé/čerpadlo. Žádný z těchto testů zde nebyl proveden.
