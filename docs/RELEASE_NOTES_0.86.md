# HOME WATER CONTROLLER — DEV KIT 0.86

Datum: 2026-11-04

## Rozšíření testů NVS credential adapteru

Revize 0.86 nemění produkční firmware ani chování CredentialStore. Rozšiřuje hostitelské testy skutečného rozhraní `NvsCredentialBackend`, aby pokrývaly poškozené a hraniční payloady.

- Test embedded NUL uvnitř uložené hodnoty; adapter musí vrátit `CREDENTIAL_READ_ERROR`.
- Test chybějícího koncového NUL; adapter musí vrátit `CREDENTIAL_READ_ERROR`.
- Test payloadu delšího než výstupní buffer; adapter musí vrátit `CREDENTIAL_READ_ERROR`.
- Test prázdného C řetězce: adapter jej syntakticky přečte, ale validační politika `CredentialStore` jej odmítne jako neplatné přihlašovací údaje.
- Test limitu délky při zápisu, nulového ukazatele, mazání chybějícího klíče, použití backendu před `begin()` a po `end()` a odmítnutí prázdného namespace.
- Testovací Preferences stub nadále není důkazem chování skutečné ESP32 NVS.

## Výsledky

- `./tests/run_native_tests.sh`: 47/47 test programů PASS.
- `sh tests/.run_native_tests_asan.sh`: 47/47 test programů PASS.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- CredentialStore/NVS backend zdroje jsou synchronizované ve `common`, `ESP32_1/src` a `ESP32_2/src`.
- ZIP integrity: PASS.

## Omezení a otevřené brány

- CredentialStore stále není zapojen do boot/AP call-sites obou `.ino` souborů.
- Fyzicky autorizovaný provisioning a obnova přístupu nejsou implementovány.
- Hesla stále existují ve výpisech Serial a AP heslo je zobrazováno na HTML dashboardu ESP32 #1; jejich odstranění by bez obnovovací cesty mohlo znemožnit správu zařízení.
- Hostitelské testy neověřují skutečnou Preferences/NVS, Arduino-ESP32 build, NVS Encryption ani Flash Encryption.
- Peer command path zůstává default-deny; fyzické výstupy jsou neaktivní.
