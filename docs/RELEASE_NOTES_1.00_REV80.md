# Release notes — DEV KIT 1.00 REV80

Datum: 17. 11. 2026 | Základ: REV79

## Změna

`NvsSensorConfigStore` nyní při `save()` i explicitním `recover()` po zápisu načte celý záznam a porovná všechny položky s kandidátem, který měl být uložen.

Předchozí běžný `save()` ověřoval délku, validitu a CRC načteného záznamu a následně porovnával CRC. REV80 zpřesňuje kontrolu na úplnou významovou shodu všech polí `SensorConfig`: magic, verze, shunt, meze a tolerance proudu, rozsah snímače, měřítko, offset a checksum.

Stejný zdrojový soubor je synchronizován do `firmware/common`, `firmware/ESP32_1/src` a `firmware/ESP32_2/src`. Test `test_nvs_sensor_config_store.cpp` ověřuje úplný obsah načtený po uložení.

## Bezpečnostní dopad

- Nemění se formát NVS, namespace, klíče ani migrace.
- Nemění se limity, výpočty hladiny, ochrany čerpadel, nouzové režimy, GPIO, relé ani ventily.
- Při neúspěšném ověření zůstává `save()`/`recover()` neúspěšné; volající musí zachovat stávající fail-closed postup.

## Ověření

- `./tests/run_native_tests.sh`: 78/78 nativních C++ testovacích programů PASS; všechny statické kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- `sh tests/.run_native_tests_asan.sh`: 76/76 nativních C++ testovacích programů PASS; statické kontroly ASan runneru PASS.
- Arduino cílový překlad a fyzické testy nebyly provedeny.
