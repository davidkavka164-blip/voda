# HOME WATER CONTROLLER — revize 0.61

Datum: 2026-11-02

## Účel revize

Revize 0.61 doplňuje úplnější kontrolu shody zdrojového projektu s HOME WATER CONTROLLER – MASTER 1.3. Firmware se nemění.

## Změny
- Přidána `MASTER_COMPLIANCE_AUDIT.md` s maticí požadavků, skutečným stavem implementace, omezeními a prioritami.
- Ověřeno, že stavové zprávy W5500/UDP nejsou autentizované a nesmějí sloužit jako řídicí kanál.
- Výslovně zaznamenáno, že horní objem a teplota jsou ve vysílaném rámci neplatné, fyzické výstupy zůstávají neaktivní a transfer je pouze simulace.
- Zaznamenán bezpečnostní problém s výpisem AP a administračních hesel do Serial Monitoru a nutnost nejprve připravit náhradní provisioning.
- Znovu spuštěno 43/43 nativních testů a 43/43 testů s AddressSanitizer/UndefinedBehaviorSanitizer.
- Ověřena bajtová shoda `firmware/common/` s kopiemi v obou jednotkách.

## Co revize neznamená
- Nejde o nový firmware ani Arduino-ESP32 build.
- Neproběhl fyzický test ESP32, W5500, čidel, NVS, Wi-Fi/AP, relé ani ventilů.
- Nejde o povolení aktivovat výkonové výstupy.
