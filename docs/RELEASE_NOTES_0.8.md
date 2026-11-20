# Release notes — 0.8

## Studna: nastavitelné rozměry a zachování kalibrace
- Přidán `WellGeometry`: výchozí vnitřní průměr 80 cm a využitelná hloubka 800 cm; oba parametry jsou odděleně nastavitelné a validované.
- Válcový výpočet je pouze orientační odhad: V = π × (D/2)² × výška / 1000. Při 80 cm a výšce 800 cm vychází přibližně 4 021 l.
- Naměřené body kalibrace mají přednost uvnitř svého rozsahu. Mimo něj se použije jen zřetelně označený geometrický odhad; bez známého absolutního kotevního bodu se objem neoznačuje jako potvrzená absolutní zásoba.
- Geometrie a naměřená kalibrace jsou dvě samostatné konfigurace. Změna průměru/hloubky nepřepisuje naměřené body.
- Přidán `NvsWellGeometryStore` a `NvsWellCalibrationStore` s verzí, validací a CRC32; obě konfigurace jsou ve vlastních klíčích namespace `hwc_well`.
- ESP32 #1 načte geometrii a uloženou křivku při startu a vypíše objem s rozlišením „orientační geometrický odhad“ vs. „naměřená kalibrace“.
- Pokud v NVS nejsou rozměry, použijí se výchozí hodnoty pouze v RAM; automaticky se neukládají. Pokud není platná kalibrace, použije se prázdná křivka a geometrický odhad.

## Ověření
- 6/6 nativních testů prošlo s `-Wall -Wextra -Werror -pedantic`.
- 6/6 nativních testů prošlo s AddressSanitizer a UndefinedBehaviorSanitizer.
- Obě Arduino sketch kostry a všechny jejich `.cpp` soubory prošly syntaktickou kontrolou proti dočasným stubům Arduino/Preferences API.
- Tato kontrola není náhradou za překlad s Arduino-ESP32 core a fyzickou zkoušku.

## Zatím nehotovo
- Webová obrazovka pro změnu rozměrů a editaci/kalibraci křivky zatím neexistuje; NVS API a datový model jsou připravené.
- Lokální překalibrace se zachováním bodů mimo měřený interval ještě není napojená do uživatelského workflow.
- Absolutní zásoba není spolehlivě známá, dokud uživatel nepotvrdí kotevní bod; geometrie zůstává odhadem i při známých rozměrech.
- Skutečný Arduino překlad, test na deskách a kontrola skutečného zapojení senzorů čekají.
