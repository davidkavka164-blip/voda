# HOME WATER CONTROLLER — revize 0.64

Datum: 2026-11-02

## Účel

Revize 0.64 dokumentuje platformní omezení při přípravě chráněného NVS backendu. Firmware se nemění.

## Zjištění
- Windows 7 omezuje dostupnou řadu Arduino-ESP32; oficiální dokumentace uvádí, že core 2.x.x není podporován, zatímco 1.0.6 je uváděna jako poslední funkční řada pro Windows 7.
- `initArduino()` v core 1.0.6 i 2.0.17 volá `nvs_flash_init()` před `setup()`.
- `Preferences` samo neaktivuje NVS Encryption; výchozí NVS spoléhá na předchozí inicializaci a custom partition cesta používá běžné `nvs_flash_init_partition()`.
- ESP-IDF v3.3 má `nvs_flash_secure_init()` a příslušné NVS encryption API, ale skutečný bezpečný postup musí být ověřen v konkrétním core/buildu a na testovací desce.
- NVS Encryption založené na Flash Encryption vyžaduje chráněnou NVS key partition.

## Změny
- Přidán `docs/NVS_ENCRYPTION_PLATFORM_AUDIT.md`.
- Aktualizován `BUILD_AND_TOOLING.md`, `SECURITY_PROVISIONING_DESIGN.md` a `IMPLEMENTATION_PLAN.md`.
- Firmware, partition table, eFuse a hardware se nemění.

## Otevřený bod

Před platformní implementací je nutné zjistit přesnou verzi Arduino-ESP32 core instalovanou v Arduino IDE 1.8.19. Do té doby není produkční `PeerSecretBackend` implementován a peer command path zůstává zakázaná.
