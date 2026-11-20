# Release notes — HOME WATER CONTROLLER REV68

Datum: 14. 11. 2026
Základ: uživatelem dodaný DEV KIT REV67

## Oprava přenositelnosti testů
- `tests/run_native_tests.sh` spouští statické shell kontroly explicitně přes `sh`, nikoli přímým spuštěním souboru.
- Opraveno i vnořené volání `check_common_module_sync.sh` v `check_upper_transfer_valve_gate.sh`.
- Stejná úprava provedena ve skriptu `.run_native_tests_asan.sh`.
- Testovací sada tak není závislá na zachování unixových executable bitů při distribuci a rozbalení ZIP archivu.

## Výsledky ověření
- `sh tests/run_native_tests.sh`: 76/76 nativních C++ testovacích programů PASS; všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- ASan/UBSan nebylo možné v tomto běhovém prostředí potvrdit: výchozí `LD_PRELOAD` způsobí konflikt pořadí ASan runtime; po jeho odebrání běhové prostředí nedokáže rezervovat ASan shadow memory. Tento výsledek není klasifikován jako PASS ani jako chyba firmware.
- Arduino cílový překlad nebyl proveden.
- Fyzické testy ESP32, čidel, relé, ventilů a kalibrace nebyly provedeny.

## Funkční rozsah
REV68 nemění firmware logiku ani hardwarové zapojení. Funkce horní objemové kalibrace z REV67 zůstává beze změny. ESP32 #2 web je stále dostupný přes servisní AP; webový přístup přes W5500/LAN není implementován.
