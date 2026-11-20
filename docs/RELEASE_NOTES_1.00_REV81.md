# Release notes — DEV KIT 1.00 REV81

Datum: 18. 11. 2026 | Základ: REV80

## Změny

1. `NvsTransferHistoryStore::save()` po zápisu ověřuje úplnou významovou shodu uloženého snapshotu s kandidátem. Porovnává metadata snapshotu a všech 12 položek včetně rezervovaných a časových polí.
2. `NvsDailyInflowStore::save()` ověřuje přímo všechny hodnoty uloženého snapshotu, nikoli pouze rovnost odvozených CRC.
3. Přidán fault-injection test, v němž backend oznámí úspěšný plný zápis, ale ponechá starý platný záznam. Ukládání historie musí tento stav odmítnout.

## Bezpečnostní dopad

- Jde o statistická/informační úložiště. Nejsou používána jako pumpové povolení, ochranný limit ani transferový restart journal.
- Při neshodě read-backu se `save()` vrátí jako neúspěšné; stávající platný záznam se nepovažuje za nový.
- Bez změny formátu NVS, klíčů, migrace, bezpečnostních prahů, GPIO, relé, ventilů a nouzových režimů.

## Ověření

- `./tests/run_native_tests.sh`: 78/78 nativních C++ testovacích programů PASS; všechny statické integrační kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS; `python3 -m compileall -q tools tests/python`: PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 nativních C++ testovacích programů PASS; všechny statické kontroly ASan runneru PASS.
- Arduino cílový překlad, skutečná ESP32 NVS a fyzické power-cut/hardware testy nebyly provedeny.
