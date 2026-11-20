# HOME WATER CONTROLLER DEV KIT 0.95

Datum: 4. 11. 2026

## Cíl revize

Připojit existující dvouslotový `CredentialStore` a jeho NVS backend do skutečných AP startup call-sites obou sketchů a bezpečně migrovat staré rozdělené NVS záznamy.

## Změny

- Přidán `CredentialLegacyMigrator.h`. Jeho `loadOrMigrate()` nejprve načte stav nového úložiště:
  - `VALID`: vrátí pouze novou uloženou sadu; legacy hodnoty ignoruje.
  - `EMPTY`: přijme jen kompletní validní legacy sadu, zapíše ji přes privátní staging/commit `CredentialStore::saveInternal()` a následně ověří celý uložený stav.
  - `CORRUPT`: odmítne migraci i legacy fallback.
  - `IO_ERROR`: odmítne migraci i legacy fallback.
- `CredentialLegacyMigrator` je jediný nový friend `CredentialStore`, který může použít interní migrační zápis. Veřejné síťové ani webové rozhraní pro něj neexistuje.
- `CredentialStore::validate()` nyní kromě délek odmítá řídicí a ne-ASCII znaky ve všech třech textových polích; tím je stejná kontrola aktivní při bootu, migraci i provisioningu.
- ESP32 #1 používá po revizi 0.95 namespace `hwcCred` jako autoritativní zdroj `apName`, `apPassword` a `adminPassword`. Legacy `hwc_ap/password` a `hwc_auth/admin` slouží pouze při prvním přechodu, pokud je nový store `EMPTY`.
- ESP32 #2 používá stejný autoritativní store. Legacy `hwc_u2ui/ap_name`, `ap_pass` a `admin` slouží pouze k jednorázové migraci při stavu `EMPTY`.
- Pokud je nový store poškozený nebo nedostupný, obě jednotky zůstanou bez AP/web credentialů. Nezkoušejí návrat ke starým namespace, protože by to mohlo obejít poškozený aktivní selector.
- Původní legacy klíče se nemažou. Zůstávají jako nepoužívané migrační záznamy; jejich mazání není součástí této revize.
- Jednorázový Serial handoff nově vytvořených hesel se provede až po úspěšném commit/read-back celé sady do `hwcCred`. Při běžném startu z již validního `hwcCred` se hesla nevypisují.
- Přidán `test_credential_legacy_migrator.cpp` a rozšířena statická kontrola obou sketch call-sites.

## Testy

- `./tests/run_native_tests.sh`: 55/55 native test programs.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.
- `tests/check_credential_module_sync.sh`: PASS.
- `tests/check_credential_startup_gate_integration.sh`: PASS.
- `sh tests/.run_native_tests_asan.sh`: PASS, 55/55 native test programs.
- `python3 -m unittest discover -s tests/python -v`: PASS, 17/17; `py_compile`: PASS.
- Peer boot identity, W5500 recovery, credential module synchronization a AP/migration static integration: PASS.
- Python klient zůstává beze změny; jeho testy se opakují v rámci závěrečné kontroly.

## Bezpečnostní hranice a omezení

- `hwcCred` ukládá credentials v plaintextu NVS. Tato revize neaktivuje NVS Encryption ani Flash Encryption.
- Legacy namespaces se zachovávají a nepoužívají jako fallback po vytvoření nového store.
- Jednorázové Serial handoff nově vytvořených hesel zůstává dočasný. Fyzicky autorizovaný servisní board adapter/Serial transport není připojen do `.ino`.
- Nebyl ověřen Arduino-ESP32 build, skutečná NVS na ESP32, power-cut migrace, UART/GPIO ani Wi-Fi SoftAP na hardware.
- Peer command path zůstává default-deny. Fyzické ventily a čerpadla zůstávají neaktivní.
