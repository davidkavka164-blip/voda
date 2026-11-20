# HOME WATER CONTROLLER — auditní balíček REV55

Datum: 14. 11. 2026

## Změny

- Přidán `docs/NVS_ENCRYPTION_STOCK_CORE_AUDIT_REV55.md`.
- Prověřen oficiální `sdkconfig.h` Arduino-ESP32 1.0.6: `CONFIG_NVS_ENCRYPTION` v něm není definováno.
- Prověřeny výchozí partition tabulky `default.csv` a `min_spiffs.csv`: neobsahují `nvs_keys` ani samostatnou peer-secret partition.
- Z toho plyne, že stock Arduino-ESP32 1.0.6 neposkytuje v běžném buildu secure-init API NVS jako dostupnou cestu bez přestavby platformy.
- Doporučeno neexperimentovat s deinit výchozí NVS; další technické posouzení směřovat k externímu secure elementu nebo samostatnému platformnímu buildu.

## Co se nemění

- Žádná změna firmware, NVS dat, partition table, eFuse, UDP komunikace ani hardware.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Horní automatický i servisní transfer zůstává blokován.

## Stav ověření

- Statická kontrola oficiálních souborů Arduino-ESP32 1.0.6: provedena.
- Arduino cílový překlad a skutečná verze core instalovaná uživatelem: nepotvrzeny.
- Flash/NVS Encryption, eFuse, OTA a hardware: neověřeny.
