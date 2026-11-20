# HOME WATER CONTROLLER — DEV KIT 1.00 REV84

Datum: 19. 11. 2026  
Základ: DEV KIT 1.00 REV83

## Změna

REV84 zavádí transakční dvouslotové ukládání domácích Wi-Fi STA údajů ESP32 #1. Nový modul `WifiStaCredentialStore` ukládá SSID a heslo společně do neaktivního slotu, provádí přesný read-back obou polí a přepíná `sta_active` až po ověření kandidáta.

Původní klíče `ssid` a `password` se migrují pouze jako úplná dvojice. Původní klíče se nemažou. Pokud se migrace přeruší, lze ji opakovat z původní dvojice. Neplatný marker nebo neúplná legacy dvojice znamená fail-closed přechod do konfiguračního AP.

## Testy

- `./tests/run_native_tests.sh`: 79/79 nativních C++ testovacích programů PASS; všechny statické integrační kontroly PASS.
- `sh tests/.run_native_tests_asan.sh`: 77/77 nativních testovacích programů PASS; statické kontroly runneru PASS.
- Python testy: 17/17 PASS.
- `compileall`, `sh -n` obou runnerů a ZIP integrita: PASS.
- Arduino cílový překlad a fyzické NVS/power-cut testy nebyly provedeny.

## Bezpečnostní rozsah

Změna se týká pouze domácích Wi-Fi STA přístupových údajů ESP32 #1. Nemění AP/admin credential store, síť ESP32 #2, pumpové ochrany, GPIO, relé, ventily, transferovou logiku ani nouzové režimy.
