# HOME WATER CONTROLLER — auditní balíček REV54

Datum: 14. 11. 2026

## Změny

- Přidán `docs/NVS_SECURE_INIT_STARTUP_AUDIT_REV54.md`.
- Provedena dokumentační kontrola Arduino-ESP32 1.0.6 `initArduino()` a `Preferences::begin()` proti skutečným zdrojům.
- Provedena kontrola ESP-IDF v3.3 secure-init/deinit implementace.
- Popsána kandidátní cesta `nvs_flash_deinit()` → `nvs_flash_secure_init()` a zásadní podmínka `CONFIG_NVS_ENCRYPTION`.
- Zkontrolováno pořadí prvních `Preferences.begin()` v obou produkčních sketchích; před nimi jsou pump safety výstupy nastaveny LOW.

## Co se nemění

- Firmware, síťová topologie, UDP porty, NVS data layout, partition table a eFuse.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Produkční handshake a autentizovaný status receiver nejsou zapojeny.

## Stav ověření

- Statická kontrola zdrojů: provedena.
- Arduino cílový překlad: neproveden.
- Skutečná Flash/NVS Encryption: neověřena.
- Hardware, eFuse provisioning, Wi-Fi, OTA a UDP handshake: neověřeno.
