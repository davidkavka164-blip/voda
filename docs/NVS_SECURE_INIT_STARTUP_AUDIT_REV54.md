# HOME WATER CONTROLLER — audit pořadí NVS secure-init REV54

Datum: 14. 11. 2026  
Vychází z: uživatelem dodaného REV52 a dokumentačního balíčku REV53  
Rozsah: statická kontrola Arduino-ESP32 1.0.6 / ESP-IDF v3.3 zdrojů a skutečného pořadí inicializace obou sketchů. Firmware, partition table, eFuse ani hardware se nemění.

## 1. Nový závěr

Existuje technicky možná cesta, jak po Arduino startupu znovu inicializovat výchozí NVS šifrovaně, ale zatím ji nelze schválit pro produkci:

1. Arduino-ESP32 1.0.6 volá `nvs_flash_init()` uvnitř `initArduino()` ještě před `setup()`.
2. ESP-IDF v3.3 poskytuje `nvs_flash_deinit()` a `nvs_flash_secure_init()`.
3. Zdroj ESP-IDF v3.3 ukazuje, že `nvs_flash_secure_init_partition()` nejprve kontroluje, zda už partition existuje v interním seznamu NVS. Pokud ano, vrací `ESP_OK` bez nastavení nového bezpečnostního kontextu.
4. Proto by pouhé zavolání `nvs_flash_secure_init()` po Arduino startupu nebylo důkazem šifrování. Pro změnu režimu by bylo nutné nejprve úspěšně zavolat `nvs_flash_deinit()` a až poté secure-init s platným `nvs_sec_cfg_t`.
5. Secure-init implementace je v ESP-IDF v3.3 podmíněna `CONFIG_NVS_ENCRYPTION`. Pokud tato volba není přítomna v konkrétně zkompilovaném core, samotný sketch potřebné API nezpřístupní.

To je návrh ověřovacího postupu, nikoli potvrzení, že dodaný firmware/core tuto cestu podporuje.

## 2. Co ukazuje skutečný zdroj Arduino-ESP32 1.0.6

Soubor `cores/esp32/esp32-hal-misc.c`, funkce `initArduino()`:

- volá `nvs_flash_init()` před `init()` a `initVariant()`;
- při `ESP_ERR_NVS_NO_FREE_PAGES` může automaticky vymazat výchozí NVS partition a zkusit inicializaci znovu.

Soubor `libraries/Preferences/src/Preferences.cpp`, funkce `Preferences::begin()`:

- pro výchozí partition volá pouze `nvs_open()`;
- pro pojmenovanou partition volá `nvs_flash_init_partition()` a pak `nvs_open_from_partition()`;
- sama o sobě nevyžaduje ani nezapíná secure-init.

## 3. Co ukazuje skutečný zdroj ESP-IDF v3.3

Soubor `components/nvs_flash/src/nvs_api.cpp`:

- `nvs_flash_secure_init_partition()` je dostupná pouze při `CONFIG_NVS_ENCRYPTION`;
- pokud je partition už nalezena v interním seznamu NVS, funkce vrátí `ESP_OK` ihned;
- `nvs_flash_deinit_partition()` odstraní storage z interního seznamu a související NVS handles;
- `nvs_flash_deinit()` je wrapper pro výchozí partition `nvs`.

Dokumentace ESP-IDF v3.3 popisuje pořadí secure NVS operací: najít NVS a key partition, načíst nebo vygenerovat bezpečnostní konfiguraci, zavolat `nvs_flash_secure_init()` / `nvs_flash_secure_init_partition()`, otevřít namespace a teprve potom číst/zapisovat. Dokumentace zároveň uvádí, že key partition musí být chráněna Flash Encryption; NVS Encryption je na tomto schématu na Flash Encryption závislá.

## 4. Skutečné pořadí v REV52 sketchích

### ESP32 #1

- `setup()` nejprve nastaví oba pump safety výstupy LOW a inicializuje je ve vypnutém stavu.
- Následuje inicializace ventilových výstupů.
- První dohledané `Preferences.begin()` v `setup()` je až volání `configStore.begin()`.
- Wi-Fi a další NVS přístupy nastávají později.

### ESP32 #2

- `setup()` nejprve nastaví horní pump safety výstup LOW a inicializuje jej ve vypnutém stavu.
- První dohledané `Preferences.begin()` v `setup()` je až volání `configStore.begin()`.
- Další NVS a Wi-Fi inicializace následují později.

To vytváří úzké časové okno, ve kterém by samostatný platformní test mohl ověřit deinit/secure-init ještě před prvním použitím `Preferences`. Statická kontrola ale sama nevylučuje skryté NVS použití v knihovnách ani nedokazuje, že je bezpečné znovu inicializovat NVS za běhu Arduino runtime.

## 5. Blokující neznámé

1. Přesná instalovaná verze Arduino-ESP32 core stále není známa.
2. Není dostupný skutečný `sdkconfig.h` / build configuration daného core, takže není potvrzeno `CONFIG_NVS_ENCRYPTION`.
3. Není potvrzena dostupnost a konfigurace `nvs_keys` partition ani její ochrana.
4. Flash Encryption není nakonfigurována ani ověřena; její aktivace může měnit chování bootloaderu, nahrávání firmware a OTA a může zahrnovat nevratné eFuse kroky.
5. Není ověřeno, jak se zachovají stávající Preferences namespaces a OTA po secure-init a opakovaných restartech.

## 6. Bezpečný další experiment

### Fáze A — pouze kontrola buildu, bez zápisu flash

- Na přesné kopii instalovaného core ověřit, zda `CONFIG_NVS_ENCRYPTION` existuje a zda jsou secure-init symboly skutečně linkovatelné.
- Sestavit malý testovací sketch, který pouze vypíše výsledek compile-time kontroly a verze ESP-IDF/core.
- Nevolat `nvs_flash_deinit()`, secure-init, erase, generate-keys ani žádnou eFuse operaci v tomto prvním kroku.

### Fáze B — oddělená testovací deska

Teprve pokud Fáze A potvrdí potřebné API, připravit samostatný test na spare ESP32 a s vlastním experimentálním partition layoutem. Neprovádět jej na provozních jednotkách. Flash Encryption/eFuse kroky vyžadují zvláštní rozhodnutí a kontrolovaný fyzický provisioning; nejsou součástí tohoto auditu.

### Fáze C — provozní validace

Před jakoukoli produkční změnou prokázat zachování všech stávajících Preferences dat, Wi-Fi reconnect, OTA update/rollback, výpadek napájení během zápisu, restart a fail-closed chování při chybě klíčové partition. Poté teprve vytvořit a testovat produkční `PeerSecretBackend`.

## 7. Rozhodnutí REV54

- Deinit + secure-init je kandidátní technická cesta, ne hotové řešení.
- Bez `CONFIG_NVS_ENCRYPTION`, správné key partition a aktivní Flash Encryption nelze backend označit jako důvěrný.
- Žádná změna firmware, partition table, eFuse, UDP transportu ani bezpečnostní brány se neprovádí.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Horní běžný automatický i servisní transfer zůstává blokován.
- Nouzové ruční otevření zůstává samostatnou potvrzovanou obslužnou výjimkou.

## 8. Zdroje

- Arduino-ESP32 1.0.6 `initArduino()`: https://raw.githubusercontent.com/espressif/arduino-esp32/1.0.6/cores/esp32/esp32-hal-misc.c
- Arduino-ESP32 1.0.6 `Preferences.cpp`: https://raw.githubusercontent.com/espressif/arduino-esp32/1.0.6/libraries/Preferences/src/Preferences.cpp
- ESP-IDF v3.3 `nvs_api.cpp`: https://raw.githubusercontent.com/espressif/esp-idf/v3.3/components/nvs_flash/src/nvs_api.cpp
- ESP-IDF v3.3 NVS documentation: https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/storage/nvs_flash.html
- ESP-IDF v3.3 Kconfig `CONFIG_NVS_ENCRYPTION`: https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/kconfig.html
