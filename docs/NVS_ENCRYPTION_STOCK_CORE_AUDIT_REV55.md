# HOME WATER CONTROLLER — audit dostupnosti NVS Encryption v Arduino core 1.0.6 REV55

Datum: 14. 11. 2026  
Vychází z: uživatelem dodaného REV52 a dokumentačních balíčků REV53–REV54  
Rozsah: statická kontrola oficiálního repozitáře Espressif `arduino-esp32` tag `1.0.6`; bez změny firmware, partition table nebo hardware.

## 1. Rozhodující nové zjištění

Oficiální `tools/sdk/include/config/sdkconfig.h` v Arduino-ESP32 1.0.6 **neobsahuje** `CONFIG_NVS_ENCRYPTION`. Kontrola úplného souboru (402 řádků) nenašla ani `CONFIG_SECURE_FLASH_ENC_ENABLED`.

ESP-IDF v3.3 podmiňuje deklarace i implementaci funkcí `nvs_flash_secure_init()` a `nvs_flash_secure_init_partition()` makrem `CONFIG_NVS_ENCRYPTION`. V dodané konfiguraci stock Arduino core 1.0.6 proto tyto funkce nejsou součástí standardního buildu. Samotné `#include <nvs_flash.h>` ani volání API z `.ino` nevytvoří chybějící implementaci.

**Závěr pro stock Arduino-ESP32 1.0.6:** cesta `nvs_flash_deinit()` → `nvs_flash_secure_init()` není dostupná bez přestavby / nahrazení platformního SDK a knihoven. Nedává smysl připravovat testovací sketch, který pouze předpokládá přítomnost této funkce.

Poznámka: release Arduino-ESP32 1.0.6 je uváděn jako založený na ESP-IDF v3.3.5. Kontrolovaná konfigurace pochází přímo z tagu 1.0.6. Skutečná verze core instalovaná u uživatele však stále není potvrzena; tento závěr je tedy přesný pro stock tag 1.0.6, nikoli automaticky pro každou instalaci.

## 2. Kontrola výchozí partition table

Oficiální soubory `tools/partitions/default.csv` a `tools/partitions/min_spiffs.csv` v tagu 1.0.6 obsahují standardní výchozí NVS partition:

`nvs, data, nvs, 0x9000, 0x5000`

Neobsahují NVS key partition typu `nvs_keys`. Neobsahují ani samostatnou partition vyhrazenou pro peer secret.

Přidání nové partition by vyžadovalo vlastní partition layout a odpovídající flashování tabulky. To samo o sobě žádné šifrování nezapne a může ovlivnit kompatibilitu stávajících dat i OTA slotů; bez přesné kontroly flash mapy se to nesmí dělat na provozních jednotkách.

## 3. Proč nepokračovat přes deinit výchozí NVS

- Arduino 1.0.6 volá `nvs_flash_init()` během `initArduino()` před `setup()`.
- `nvs_flash_deinit()` odregistruje výchozí NVS storage a odstraňuje související handles. Není to bezpečný obecný způsob, jak přepnout režim NVS uprostřed běžící aplikace.
- `Preferences::begin()` pro výchozí partition volá `nvs_open()`; `Preferences` samo šifrování nezapíná.
- Při chybě `ESP_ERR_NVS_NO_FREE_PAGES` může startup Arduino core 1.0.6 automaticky vymazat výchozí NVS partition a zkusit ji inicializovat znovu. Tento mechanismus je další důvod, proč nepoužívat pokusné zásahy do výchozí NVS.

## 4. Bezpečné možnosti dalšího postupu

### Varianta A — zachovat současný Arduino core

Prověřit externí secure element, například zařízení s bezpečným slotem pro HMAC klíč. Klíč by se nesměl exportovat do ESP32; autentizační operace by musely být prováděny uvnitř secure elementu. Před výběrem konkrétní součástky je nutné ověřit, zda její příkazy a knihovna pokryjí současný handshake i odvození session key. Samotné uložení tajemství v běžné EEPROM/Preferences není přijatelné.

### Varianta B — změnit platformní build

Připravit samostatný ESP-IDF / Arduino-ESP32 build s ověřeným `CONFIG_NVS_ENCRYPTION`, key partition a Flash Encryption. Jde o zásadní změnu platformy, partition layoutu, provisioning procesu, OTA a servisního nahrávání. Flash Encryption může zahrnovat nevratné eFuse kroky. Nesmí se zkoušet na provozních jednotkách.

### Varianta C — dočasně ponechat blokaci

Pokud není možné zvolit A nebo bezpečně realizovat B, ponechat `PeerSecretVault` v režimu bez dostupného důvěrného úložiště. Autentizované ovládání horního transferu zůstane zakázané. To je současný bezpečný stav.

## 5. Doporučené rozhodnutí

Pro současný projekt se nedoporučuje experimentovat s `nvs_flash_deinit()` ani přidávat NVS partition do provozní flash mapy.

Nejprve zjistit přesnou instalovanou verzi Arduino-ESP32 a ověřit, zda odpovídá stock 1.0.6. Pokud ano, další práce se má soustředit na technické posouzení secure elementu (HMAC operace, počet slotů, provisioning, dostupnost knihovny a možnost bezpečného odvození session key). Pokud je verze jiná, nejprve se zkontroluje její skutečný `sdkconfig.h`, knihovna `nvs_flash` a partition table.

## 6. Co zůstává beze změny

- Firmware, NVS layout, partition table, eFuse a hardware nebyly změněny.
- Produkční handshake ani autentizovaný receiver nejsou zapojeny do UDP smyček.
- `PEER_STATUS_AUTHENTICATED_CONTROL_ENABLED` zůstává `false`.
- Běžný automatický i servisní horní transfer zůstává blokován.
- Nouzové ruční otevření zůstává samostatnou explicitně potvrzovanou obslužnou výjimkou.

## 7. Ověření

- Oficiální `arduino-esp32` tag 1.0.6: `sdkconfig.h` prohledán celý; `CONFIG_NVS_ENCRYPTION` nenalezen.
- Oficiální `default.csv` a `min_spiffs.csv`: nalezena pouze standardní `nvs` partition, nikoli `nvs_keys`.
- Zdrojové soubory projektu nebyly změněny.
- Arduino cílový překlad, čip, eFuse, Flash Encryption, NVS, Wi-Fi, OTA a hardware nebyly testovány.

## 8. Zdroje

- Arduino-ESP32 1.0.6 `sdkconfig.h`: https://github.com/espressif/arduino-esp32/blob/1.0.6/tools/sdk/include/config/sdkconfig.h
- Arduino-ESP32 1.0.6 `default.csv`: https://github.com/espressif/arduino-esp32/blob/1.0.6/tools/partitions/default.csv
- Arduino-ESP32 1.0.6 `min_spiffs.csv`: https://github.com/espressif/arduino-esp32/blob/1.0.6/tools/partitions/min_spiffs.csv
- Arduino-ESP32 1.0.6 `initArduino()`: https://github.com/espressif/arduino-esp32/blob/1.0.6/cores/esp32/esp32-hal-misc.c
- Arduino-ESP32 1.0.6 `Preferences::begin()`: https://github.com/espressif/arduino-esp32/blob/1.0.6/libraries/Preferences/src/Preferences.cpp
- ESP-IDF v3.3 NVS API: https://github.com/espressif/esp-idf/blob/v3.3/components/nvs_flash/src/nvs_api.cpp
- ESP-IDF v3.3 NVS Encryption documentation: https://docs.espressif.com/projects/esp-idf/en/v3.3/api-reference/storage/nvs_flash.html
- Arduino-ESP32 1.0.6 release basis ESP-IDF v3.3.5: https://newreleases.io/project/github/espressif/arduino-esp32/release/1.0.6
