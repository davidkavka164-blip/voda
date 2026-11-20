# Release notes — DEV KIT 1.00 REV82

Datum: 18. 11. 2026 | Základ: REV81

## Změny

1. `NvsWellCalibrationStore::save()` porovnává po read-backu všechna významová pole kalibrace studny: metadata, absolutní kotvu, rezervovaná pole, checksum a všechny aktivní kalibrační body. `mergeAndSave()` používá totožnou kontrolu.
2. `NvsWellGeometryStore::save()` porovnává po read-backu magic, verzi, vnitřní průměr, využitelnou hloubku a checksum.
3. Obě úložiště mají fault-injection test. Simulovaný backend oznámí plný zápis, ale ponechá starý platný záznam; `save()`/`mergeAndSave()` musí vrátit neúspěch a původní data musí zůstat čitelná.
4. Testovací běhové skripty používají pro tyto dvě NVS třídy dedikovaný stub s řízeným ignorováním jednoho zápisu.

## Bezpečnostní dopad

- Změna se týká pouze ukládání objemové kalibrace a geometrického modelu studny. `WellCalibration` zůstává výhradně informativní objemovou vrstvou a není vstupem do pumpových permitů ani transferové ochrany.
- Nezměnil se formát NVS, klíče, migrace, výchozí geometrie, bezpečnostní limity, GPIO, relé, ventily ani nouzové režimy.
- Porovnávají se významová pole, nikoli surová paměť struktur s možným paddingem.

## Ověření

- `./tests/run_native_tests.sh`: 78/78 nativních C++ testovacích programů PASS; všechny statické kontroly PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 nativních C++ testovacích programů PASS; všechny statické kontroly ASan runneru PASS.
- Python sada, `compileall` a shell syntax kontrola budou před vydáním znovu spuštěny.
- Arduino cílový překlad, skutečná ESP32 NVS a fyzické power-cut/hardware testy nebyly provedeny.
