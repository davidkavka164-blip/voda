# HOME WATER CONTROLLER — auditní balíček REV56

Datum: 14. 11. 2026

## Změny

- Přidán `docs/PEER_SECURE_ELEMENT_COMPATIBILITY_AUDIT_REV56.md`.
- Provedena statická kontrola handshake HMAC vstupů a jejich délek.
- Ověřeno, že HMAC zprávy mají 66 B, 63 B a 61 B; kandidátní ATECC608C SHA/HMAC přijímá vstupy po blocích do 64 B.
- Popsán nutný refaktor z `PeerSecretVault::load()` na HMAC provider, aby dlouhodobý secret nebyl exportován do RAM ESP32.
- ATECC608C označen pouze jako kandidátní rodina; žádná konkrétní varianta nebyla vybrána ani objednána.

## Co se nemění

- Firmware, piny, I²C, NVS, partition table, eFuse, UDP transport a bezpečnostní brány beze změny.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.

## Stav ověření

- Statická kontrola zdrojů a datového toku: provedena.
- Secure element, CryptoAuthLib, I²C, Arduino target build, provisioning a reálný handshake: neověřeny.
