# HOME WATER CONTROLLER — vývojový balíček 0.9

## Změna proti 0.8
- Přidán `WellCalibrationEditor` pro lokální překalibraci studny.
- Nové body nahrazují staré body jen v intervalu od prvního po poslední nový bod včetně.
- Původní body pod a nad tímto intervalem se zachovají beze změny.
- Sloučení se odmítne při neplatné geometrii/křivce, bodech za využitelnou hloubkou, překročení kapacity nebo nemonotónním objemu.
- Ochrana proti smíchání známého absolutního objemového základu s novými body bez potvrzených absolutních objemů.
- Nový modul je zkopírován do `src` obou Arduino sketchů.

## Kontrola
- 7/7 nativních testů prošlo.
- 7/7 nativních testů prošlo při kompilaci s AddressSanitizer a UndefinedBehaviorSanitizer.
- ZIP integrita byla ověřena. Skutečný překlad Arduino-ESP32 nebyl proveden, protože toolchain není v tomto prostředí dostupný.

## Zbývá
- Napojit kalibraci na webové rozhraní a NVS zápis.
- Přidat historii přítoku a její persistenci.
- Implementovat web, Wi-Fi/AP, W5500 a komunikaci mezi jednotkami.
- Skutečný překlad v Arduino IDE a test na hardware.
