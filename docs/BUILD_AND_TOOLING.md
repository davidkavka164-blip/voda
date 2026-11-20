# Vývojové prostředí a sestavení

## Zjištěný stav
- Uživatel používá Windows 7 SP1 a Arduino IDE 1.8.19.
- Oficiální dokumentace Arduino-ESP32 uvádí, že core 2.x.x není podporován na Windows 7 a starším; řada 1.0.6 je uváděna jako poslední funkční pro Windows 7. Přesná instalovaná verze core zatím není známa.
- NVS Encryption proto nelze navrhovat podle novějšího ESP-IDF bez ověření skutečného core. Viz `NVS_ENCRYPTION_PLATFORM_AUDIT.md`.
- V tomto pracovním prostředí nebylo nalezeno `arduino-cli`, `arduino` ani `xtensa-esp32-elf-g++`.
- Překlad Arduino/ESP32 firmware proto zatím nelze v tomto prostředí ověřit.
- Přenosné moduly lze testovat běžným C++ kompilátorem bez Arduino knihoven; revize 0.59 má 43/43 nativních testovacích programů a celá sada prošla také pod AddressSanitizer a UndefinedBehaviorSanitizer.
- Oba sketchy a platformní moduly prošly také syntaktickou kontrolou proti dočasným stubům Arduino/Preferences API. Toto není náhrada skutečného překladu s balíčkem desky ESP32.

## Zvolená strategie
- Nezávislé algoritmy bez hardwarových závislostí vyvíjet a testovat nativně.
- Arduino-specifické moduly připravovat s ohledem na Arduino IDE 1.8.19 a C++11.
- Pro aktuální čtení ADC se používá `analogReadMilliVolts()` a `analogSetPinAttenuation()`; při prvním skutečném překladu ověřit kompatibilitu s instalovaným Arduino-ESP32 core.
- Před označením firmware za přeložený je nutné ověřit přesnou verzi ESP32 board package a knihoven v uživatelově Arduino IDE a provést překlad obou `.ino` projektů.
- Codex desktop neinstalovat na Windows 7. Není blokátorem vývoje: zdrojové soubory lze připravit a testovat zde a následně předat jako ZIP. Použití jiného počítače nebo cloudového nástroje je volitelné, nikoli podmínkou realizace.

## Další knihovní závislost revize 0.16
- Pro W5500 peer status je potřeba Arduino knihovna `Ethernet` s podporou `Ethernet.begin(mac, timeout, responseTimeout)`, `EthernetUDP` a standardní `SPI` knihovna dodaná s ESP32 core.
- W5500 je nastaven na DHCP s limitem 5 s a pokouší se o obnovu inicializace přibližně po 60 s, pokud DHCP při startu selže.
- Tato knihovní kombinace nebyla přeložena ve skutečném Arduino IDE; před použitím ověřit kompatibilitu verze Ethernet knihovny s Arduino-ESP32 core.


## Credential RNG platformní předpoklad (revize 0.96)
- Při generování nových AP/admin tajemství se před `esp_random()` vyžaduje úspěšné `WiFi.mode(WIFI_STA)` po `WiFi.persistent(false)`. Tím se aktivuje Wi-Fi RF subsystém, který podle ESP-IDF 3.3.x poskytuje entropy pro true-random RNG výstupy.
- Pro Arduino-ESP32 1.0.6 upstream `WiFiGenericClass::mode()` vrací `bool` a při přechodu z NULL inicializuje Wi-Fi driver a spouští `esp_wifi_start()`. Předpoklad je ověřen ve zdrojovém tagu 1.0.6, ne však aktuálním fyzickým buildem.
- Nové výchozí AP/admin heslo obsahuje 32 hex znaků (4×32bit RNG slovo). Při chybě startu Wi-Fi se nové credentials negenerují.
- Odkazy: ESP-IDF v3.3.4 Random Number Generation https://espressif-docs.readthedocs-hosted.com/projects/esp-idf/en/v3.3.4/api-reference/system/system.html ; Arduino-ESP32 1.0.6 WiFiGeneric.cpp https://raw.githubusercontent.com/espressif/arduino-esp32/1.0.6/libraries/WiFi/src/WiFiGeneric.cpp .


## DS18B20 knihovny REV69
- ESP32 #2 používá knihovny `OneWire` (Paul Stoffregen) a `DallasTemperature` (Miles Burton).
- Knihovny je nutné nainstalovat samostatně v Arduino IDE 1.8.19. Přesné verze zatím nejsou připnuty ani ověřeny proti instalovanému Arduino-ESP32 core, protože jeho verze není známa.
- REV69 hostitelsky kompiluje a testuje společný validační modul `WaterTemperature`; tato kontrola nekompiluje knihovní API ani celý Arduino sketch.
- W5500 webová knihovna `WebServer_ESP32_W5500` uvádí ESP32 core 2.0.6+ jako předpoklad; bez potvrzení verze core ji do projektu nepřidávat.
