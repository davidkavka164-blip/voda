# HOME WATER CONTROLLER — návazný audit peer autentizace REV53

Datum: 14. 11. 2026  
Vychází z: uživatelem dodaného archivu `HOME_WATER_CONTROLLER_DEV_KIT_1.00_AUDIT_DRAFT_REV52(1).zip`  
Rozsah: statická kontrola skutečných `.ino` call-sites, bezpečnostní brány horní cesty, testů REV52 a platformních předpokladů pro bezpečné úložiště peer tajemství. Firmware, NVS layout, partition table a eFuse se nemění.

## 1. Shrnutí

REV52 přidává `PeerAuthenticatedStatusReceiver`, který ověřuje HMAC statusu před publikací do `PeerStatusMonitor`. Modul je přenositelný a testovaný, ale hlavní UDP smyčka ESP32 #1 jej dosud nevolá. ESP32 #2 dosud nevykonává příjem handshake paketů. Produkční UDP status proto zůstává CRC-only.

Bezpečnostní brána horní cesty je v REV52 skutečně fail-closed:

- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` je `false`.
- Běžný automatický transfer horní → dolní vyžaduje tuto konstantu.
- Běžný servisní ruční ventilový režim horní → dolní ji také vyžaduje.
- Nouzové ruční otevření horní cesty je samostatná výslovná obslužná výjimka a není vydáváno za autentizovaný síťový provoz.
- CRC-only peer rámce mohou zůstat diagnostickou telemetrií, ale nemohou povolit běžné ani servisní ovládání horního ventilu.

## 2. Přímé nálezy v REV52

### ESP32 #1

- `pollPeerWifi()` v `firmware/ESP32_1/ESP32_1.ino` naslouchá na UDP 4211.
- Přijímá přesně `PEER_STATUS_WIRE_SIZE`, volá `decodePeerStatusFrame()` a předává výsledek do `peerStatusMonitor.accept()`.
- Funkce nevolá `PeerAuthenticatedStatusReceiver`, `TimedPeerHandshakeInitiator`, `PeerSecretVault` ani autentizovaný session codec.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED == false` je použito v běžné transferové i servisní ventilové autorizační cestě.

### ESP32 #2

- `startPeerEthernet()` otevírá UDP socket na portu 4212.
- `sendPeerStatus()` vysílá 52B CRC status broadcastem na `255.255.255.255:4211`.
- Hlavní `.ino` nevolá `parsePacket()` pro handshake; socket je v současném firmware vysílací.

### Bezpečnostní moduly

- `PeerAuthenticatedStatusReceiver` existuje v `firmware/common/` a obou `src/` stromů.
- Testy ověřují odmítnutí bez session, HMAC ověření před publikací, odmítnutí duplicitní sekvence a nezměnění monitoru při neautentizovaném vstupu.
- `PeerSecretVault` odmítá čtení/zápis, pokud backend nepotvrdí `confidentialStorageAvailable()`; prosté Preferences/NVS bez prokázaného šifrování nesmí být backendem označeno za důvěrné.
- Produkční důvěrný backend peer tajemství není implementován.

## 3. Platformní závěr k NVS Encryption

Audit `docs/NVS_ENCRYPTION_PLATFORM_AUDIT.md` zůstává věcně správným omezením: samotné `Preferences.begin()` nezapíná šifrování NVS. ESP-IDF v3.3 dokumentuje `nvs_flash_secure_init()` / `nvs_flash_secure_init_partition()` a vyžaduje bezpečnostní konfiguraci a klíčovou partition. NVS Encryption v popsaném schématu závisí na Flash Encryption a chráněné key partition.

Z toho neplyne, že řešení je na daném zařízení nemožné. Plyne z toho, že jej nelze zapnout pouze změnou třídy `PeerSecretVault` nebo uložením tajemství do dalšího Preferences namespace. Musí se ověřit přesný Arduino-ESP32 core, způsob inicializace výchozí NVS před `setup()`, partition table, bootloader, OTA kompatibilita a skutečné nastavení Flash/NVS Encryption.

REV52 neobsahuje `platformio.ini`, vlastní partition CSV, `sdkconfig` ani potvrzenou verzi instalovaného core. Proto se nesmí provést změna partition/eFuse ani označit NVS backend jako bezpečný.

## 4. Rozhodnutí dalšího postupu

1. Zachovat síťovou topologii a stávající UDP porty.
2. Nezapojovat handshake do produkčního UDP, dokud není vyřešené a ověřené uložení dlouhodobého peer tajemství.
3. Nejprve vytvořit oddělený platformní spike pro přesně identifikovaný Arduino-ESP32 core, bez zásahu do provozní partition table a eFuse.
4. Po prokázání bezpečného úložiště zapojit omezený handshake na již otevřených UDP socketech.
5. Poté připojit autentizovaný status receiver před `PeerStatusMonitor` a doplnit integrační testy ztrát, duplicit, timeoutů, restartů a ztráty linky.
6. `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` smí být změněno na `true` až po cílovém buildu a ověření kompletní cesty; nativní testy samy nestačí.

## 5. Testy spuštěné při auditu REV53

- `tests/run_native_tests.sh`: 70/70 nativních testovacích programů PASS.
- `tests/.run_native_tests_asan.sh`: 68/68 ASan/UBSan testovacích programů PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `tests/check_peer_auth_transport_not_integrated.sh`: PASS.
- `tests/check_peer_status_auth_control_gate.sh`: PASS.
- `tests/check_common_module_sync.sh`: PASS.
- `tests/check_mixed_wifi_ethernet_peer_transport.sh`: PASS.
- ZIP vstupní archiv: integrita PASS.

Neprovedeno: Arduino cílový překlad, skutečný NVS/Flash Encryption test, fyzický provisioning, skutečný UDP handshake a hardware testy.

## 6. Závěr

REV52 je platný mezikrok. Přijímací bezpečnostní hranice je připravena a otestována jako samostatný modul; produkční UDP ji zatím nepoužívá. Horní automatická a běžná servisní cesta je v aktuálním zdroji blokována. Nejbližší technický blokátor zůstává platformně prokázané důvěrné uložení společného tajemství.
