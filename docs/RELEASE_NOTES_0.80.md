# Release notes — DEV KIT 0.80

Datum: 2026-11-04

## Změny
- Opraven chybný závěr revize 0.79: skutečné Wi-Fi/AP/web call-sites jsou v obou hlavních `.ino` souborech a jsou nyní přímo auditovány.
- Přidán `SECURITY_CREDENTIAL_CALLSITE_AUDIT_0.80.md` s přesnými místy práce s AP/admin/Wi-Fi credentials.
- Potvrzeny P0 nálezy: AP start po neověřeném uložení, ignorované `putString` na ESP32 #2, výpisy tajemství do Serial Monitoru, heslo AP v HTML ESP32 #1 a HTTP Basic Auth bez šifrování transportu.
- Aktualizován implementační plán, stav projektu a test/review log.

## Firmware
Beze změny. Revize 0.80 je auditní a dokumentační; nezavádí izolovaný CredentialStore ani netvrdí, že P0 je uzavřeno.

## Testy
- `tests/run_native_tests.sh`: 45/45.
- `sh tests/.run_native_tests_asan.sh`: 45/45.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.
- ZIP integrity: PASS.

Arduino-ESP32 build, NVS/Flash Encryption a fyzické testy nejsou provedeny.
