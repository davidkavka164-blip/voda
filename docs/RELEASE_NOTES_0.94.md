# HOME WATER CONTROLLER DEV KIT 0.94

Datum: 2026-11-04

## Credential startup gate — legacy NVS call-sites

### Nová společná logika

- Přidán `CredentialStartupGate.h` se společným rozhraním `CredentialTextStorage` a funkcí `ensureCredentialText()`.
- Existující uložené údaje se pouze validují a načtou; neplatné existující záznamy se automaticky nepřepisují.
- Chybějící údaj lze vytvořit z nově generované hodnoty pouze po úspěšném zpětném načtení a přesné shodě.
- Návratová hodnota zápisu sama o sobě nerozhoduje: pokud backend vrátí chybu, ale přesná hodnota je v NVS přítomna, je read-back autoritativní.
- Přidán `NvsCredentialTextStorage.h` pro stávající `Preferences` namespace. Adapter explicitně rozlišuje aktivní/uzavřený stav; po `close()` vrací chybu čtení, nikoli falešný stav EMPTY.
- Oba moduly jsou bajtově shodné v `firmware/common/` a v `firmware/ESP32_1/src/` i `firmware/ESP32_2/src/`.

### ESP32 #1

- Konfigurační AP password se načítá/zakládá přes startup gate v původním namespace `hwc_ap`, klíč `password`.
- Pokud NVS nejde otevřít, hodnota je mimo limit 8–63 ASCII znaků nebo read-back nesouhlasí, `startConfigAp()` AP odmítne spustit. Přestává se používat dočasné heslo pouze v RAM.
- AP start navíc vyžaduje platné administrátorské heslo 12–64 znaků z `hwc_auth/admin`.
- Administrátorské heslo se při vytvoření ověří read-backem; poškozený existující záznam se nepřepíše.
- Heslo AP bylo odstraněno z HTML dashboardu. Dashboard stále zobrazuje SSID a adresu AP.
- Uložení domácí Wi-Fi (`hwc_wifi`) nyní ověřuje přesnou shodu obou hodnot po zápisu před restartem.

### ESP32 #2

- Před `WiFi.softAP()` se ověřují tři stávající klíče v `hwc_u2ui`: `ap_name` (8–32), `ap_pass` (8–63) a `admin` (12–64).
- AP a místní web se nespustí, pokud NVS není dostupná, kterýkoli záznam chybí a nelze ho vytvořit/ověřit, nebo je existující záznam neplatný.
- Automatické přepsání poškozených existujících údajů je zakázáno.

### Předání prvotních údajů a zbývající omezení

- Nově vytvořené AP/admin heslo se zatím jednorázově zobrazí v Serial Monitoru až po read-back ověření. Opakované vypisování existujících hesel při každém startu bylo odstraněno.
- Toto je přechodné předání přístupových údajů. Jejich úplné odstranění ze Serial logů musí počkat na integraci a fyzické ověření provisioning transportu; UART/GPIO není v této revizi připojen.
- Nová gate zatím používá původní NVS namespace/klíče. Není to migrace do dvouslotového `CredentialStore`/`hwcCred` a neposkytuje šifrování dat v NVS.
- Při existujícím poškozeném/neudržitelném záznamu se AP bezpečně nezapne; obnova přes sériový provisioning zatím není v `.ino` připojena.

## Testy a ověření

- `./tests/run_native_tests.sh`: **53/53 native test programs PASS**.
- `sh tests/.run_native_tests_asan.sh`: **53/53 native test programs PASS** (AddressSanitizer + UndefinedBehaviorSanitizer).
- `python3 -m unittest discover -s tests/python -v`: **17/17 PASS**.
- Python `py_compile`: PASS.
- `tests/check_credential_module_sync.sh`: PASS — osm credential modulů je bajtově totožných ve všech třech stromech.
- `tests/check_credential_startup_gate_integration.sh`: PASS — kontrola obou AP call-sites a HTML úniku.
- Peer boot identity: PASS.
- W5500 recovery: PASS.
- ZIP integrita: PASS (`unzip -t`); standardní 53/53, ASan/UBSan 53/53 a Python 17/17 byly zopakovány z čerstvě rozbalené kopie a prošly.

## Neprovedeno

- Arduino-ESP32 build obou `.ino` v uživatelově Arduino IDE 1.8.19 / cílové verzi core.
- Reálná Preferences/NVS, power-cut test, NVS/Flash Encryption.
- Fyzický UART, DTR/RTS, fyzický servisní vstup/GPIO a provisioning.
- Žádný peer command nebyl povolen; žádné relé, ventily ani čerpadla nebyly aktivovány.
