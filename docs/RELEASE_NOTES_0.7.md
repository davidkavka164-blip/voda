# Release notes — 0.7

## Nové
- `SensorConfig`: elektrická konfigurace 4–20 mA, validace rozsahů a CRC32 kontrola uloženého záznamu.
- `NvsSensorConfigStore`: načtení a uložení verzované konfigurace do NVS namespace `hwc_cfg`.
- `LevelSensorReader`: ADC vstup, průměr osmi měření napětí a převod přes `SensorMath`.
- ESP32 #1 vypisuje každé 2 s hodnoty snímače studny a dolní nádrže. ESP32 #2 vypisuje hladinový snímač horní nádrže.
- Nativní test konfigurace snímače.

## Ověření
- 5/5 nativních testů prošlo s `-Wall -Wextra -Werror -pedantic`.
- 5/5 nativních testů prošlo také s AddressSanitizer a UndefinedBehaviorSanitizer.
- Oba sketchy a platformní moduly prošly syntaktickou kontrolou proti dočasným Arduino/Preferences API stubům.
- Kopie společných souborů v obou `src` adresářích odpovídají `firmware/common`.
- ZIP integrity check bude proveden po zabalení.

## Co není ověřeno / není hotovo
- Nebyl proveden skutečný překlad s Arduino-ESP32 board package.
- Nebyl proveden test na fyzickém ESP32 ani se skutečnými snímači.
- NVS vrstva umí záznam uložit/načíst, ale zatím není připojena ke kalibračnímu webovému rozhraní; běžný firmware defaulty automaticky do NVS neukládá.
- Nejsou aktivní W5500, DS18B20, ventily ani relé čerpadel.
- Výchozí elektrická kalibrace 150 Ω / 4–20 mA / 10 m je předběžná a před použitím se musí ověřit skutečné zapojení a měření.
