# HOME WATER CONTROLLER — vývojový balíček 0.10

## Změna proti 0.9
- `NvsWellCalibrationStore::mergeAndSave()` propojuje validované lokální sloučení s uložením kalibrace do NVS.
- Chybějící záznam je považován za prázdnou kalibraci; existující neplatný/poškozený záznam se automaticky nepřepisuje.
- Po zápisu se záznam znovu načte a porovná se kontrolní součet a počet bodů.
- Přidán hostitelský `Preferences` stub a test pro zápis, opětovné načtení, merge a poškozený záznam.

## Kontrola
- 8/8 nativních testů prošlo.
- 8/8 nativních testů prošlo s AddressSanitizer a UndefinedBehaviorSanitizer.
- ZIP integrita bude ověřena při vytvoření balíčku. Skutečný překlad Arduino-ESP32 a fyzický test NVS na ESP32 nebyly provedeny.

## Zbývá
- Webová obrazovka pro nastavení rozměrů studny a kalibraci, napojená na validaci a NVS.
- Historie denního přítoku a persistence.
- Web, Wi-Fi/AP, W5500, komunikace mezi jednotkami, teplotní čidlo a bezpečnostní integrace.
