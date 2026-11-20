# HOME WATER CONTROLLER — platformní audit NVS Encryption

Revize: 0.64  
Datum: 2026-11-02  
Stav: dokumentační ověření; žádný firmware, partition table ani eFuse se nemění.

## 1. Výchozí prostředí projektu

Projekt je vyvíjen na Windows 7 SP1 s Arduino IDE 1.8.19. Přesná verze balíčku desek `esp32` zatím není potvrzena. V pracovním archivu není `platformio.ini`, vlastní partition CSV ani `sdkconfig`/`sdkconfig.defaults`.

Oficiální dokumentace Arduino-ESP32 uvádí, že řada core 2.x.x není podporována na Windows 7 a starším; pro Windows 7 je uváděna řada 1.0.6. To je omezení toolchainu, nikoli Arduino IDE 1.8.19 samotného.

## 2. Co dělá Arduino core s NVS

Kontrola oficiálních zdrojů Espressif:

- Arduino-ESP32 1.0.6: `initArduino()` volá `nvs_flash_init()` ještě před `setup()`. Zdroj: `cores/esp32/esp32-hal-misc.c`, tag `1.0.6`.
- Arduino-ESP32 1.0.6: `Preferences::begin()` pro výchozí NVS volá `nvs_open()`; při zadaném vlastním labelu volá `nvs_flash_init_partition()` a následně `nvs_open_from_partition()`. Zdroj: `libraries/Preferences/src/Preferences.cpp`, tag `1.0.6`.
- Arduino-ESP32 2.0.17 má stejný základní vzor: `initArduino()` volá `nvs_flash_init()`; `Preferences` pro vlastní partition volá běžné `nvs_flash_init_partition()`, nikoli samo o sobě `nvs_flash_secure_init_partition()`.

Důsledek: `Preferences` není přepínač NVS Encryption. U výchozí partition závisí šifrování na tom, jak byla NVS inicializována a jaké `CONFIG_NVS_ENCRYPTION`/Flash Encryption volby obsahuje konkrétní build. Vlastní partition nelze označit za šifrovanou jen tím, že ji otevře `Preferences.begin()`.

## 3. API ESP-IDF a bezpečnostní závislosti

ESP-IDF v3.3 (relevantní pro starší Arduino core 1.0.6) již dokumentuje:

- `nvs_flash_secure_init()` a `nvs_flash_secure_init_partition()`;
- `nvs_flash_generate_keys()` a `nvs_flash_read_security_cfg()`;
- `nvs_sec_cfg_t` pro XTS klíče;
- samostatnou NVS key partition typu `data`, subtype `key`, velikost 4096 B a příznak `encrypted`;
- závislost NVS Encryption na Flash Encryption, která chrání partition s NVS klíči.

ESP-IDF 4.4/5.x dokumentuje pro výchozí partition také automatickou inicializaci šifrování přes `nvs_flash_init()`, pokud je build nakonfigurován s `CONFIG_NVS_ENCRYPTION`. Bez potvrzení této konfigurace nelze automatickou ochranu předpokládat.

NVS Encryption chrání důvěrnost a integritu položek proti neautorizované změně bez klíčů, ale nechrání před vymazáním celé partition a sama o sobě neřeší návrat starší kopie flash.

## 4. Dopad na HOME WATER CONTROLLER

Oba sketch projekty používají `Preferences` pro více provozních dat. Před použitím stejné výchozí NVS partition pro peer tajemství musí být prokázáno, že:

1. Flash Encryption je skutečně aktivní v bezpečně zvoleném režimu a příslušná key partition je chráněna.
2. NVS Encryption je skutečně aktivní pro výchozí NVS partition.
3. Arduino startup a `Preferences` používají již bezpečně inicializovanou partition; žádná cesta ji znovu neotevře nešifrovaným init.
4. Po restartu, výpadku napájení a OTA zůstávají všechna dosavadní nastavení čitelná a šifrovaná.
5. Backend `PeerSecretVault` umí rozpoznat neaktivní nebo chybnou ochranu a v takovém případě vrátí `confidentialStorageAvailable() == false`.

U core 1.0.6 volá Arduino startup `nvs_flash_init()` před `setup()`. ESP-IDF v3.3 poskytuje samostatné secure-init API, ale jeho bezpečné zapojení do již inicializovaného Arduino runtime musí být nejprve ověřeno na testovací desce. Zatím se nepředpokládá, že lze bezpečně provést `nvs_flash_deinit()` a následný secure init bez vedlejších dopadů na Wi-Fi, Preferences a OTA.

## 5. Varianty dalšího postupu

### A — zachovat Windows 7 a Arduino core 1.0.6

Nejprve zjistit přesnou instalovanou verzi core. Následně ověřit dostupné ESP-IDF 3.3 NVS secure-init API a vytvořit oddělený testovací sketch/build. Flash Encryption stále vyžaduje správný bootloader, partition table a řízený fyzický provisioning eFuse. Bez těchto kroků se NVS backend nesmí označit jako chráněný.

### B — použít novější vývojové prostředí

Arduino-ESP32 2.x/novější core vyžaduje novější Windows než Windows 7. Ani samotný přechod na novější OS však automaticky nezapne Flash Encryption/NVS Encryption; musí být zajištěna odpovídající konfigurace buildu, bootloaderu, partition table a OTA.

### C — posoudit externí bezpečnostní prvek

Pokud nebude možné bezpečně a opakovatelně sestavit Flash Encryption/NVS Encryption pro stávající Arduino workflow, lze samostatně posoudit externí secure element. To by vyžadovalo změnu kryptografického rozhraní tak, aby se tajemství nikdy nevracelo do RAM ESP32, a potvrzení vhodného modulu, I2C pinů a provisioning procesu. Tato varianta není zatím vybrána ani implementována.

## 6. Rozhodnutí revize 0.64

- Žádná z variant není zatím vybrána jako produkční řešení.
- Nepřidávat `nvs_keys` partition naslepo a neměnit existující partition layout.
- Nevolat žádné eFuse/Flash Encryption/Secure Boot provisioning příkazy.
- Neimplementovat produkční backend pouze na základě `Preferences`.
- Peer command path a fyzické výstupy zůstávají zakázané.

**Další nezbytný údaj před platformní implementací:** přesná verze balíčku `esp32` instalovaná v Arduino IDE 1.8.19. Zjistí se v `Nástroje → Vývojová deska → Správce desek… → esp32 by Espressif Systems` (řádek Installed). Než bude tento údaj znám, lze pokračovat pouze v přenositelných modulech a dokumentaci.

## 7. Oficiální zdroje

- Arduino-ESP32 installation / Windows support: https://docs.espressif.com/projects/arduino-esp32/en/latest/installing.html
- Arduino-ESP32 1.0.6 `initArduino()`: https://raw.githubusercontent.com/espressif/arduino-esp32/1.0.6/cores/esp32/esp32-hal-misc.c
- Arduino-ESP32 1.0.6 `Preferences`: https://raw.githubusercontent.com/espressif/arduino-esp32/1.0.6/libraries/Preferences/src/Preferences.cpp
- Arduino-ESP32 2.0.17 `initArduino()`: https://raw.githubusercontent.com/espressif/arduino-esp32/2.0.17/cores/esp32/esp32-hal-misc.c
- Arduino-ESP32 2.0.17 `Preferences`: https://raw.githubusercontent.com/espressif/arduino-esp32/2.0.17/libraries/Preferences/src/Preferences.cpp
- ESP-IDF v3.3 NVS API and security: https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/storage/nvs_flash.html
- ESP-IDF v4.4 NVS Encryption: https://docs.espressif.com/projects/esp-idf/en/v4.4/esp32/api-reference/storage/nvs_flash.html
