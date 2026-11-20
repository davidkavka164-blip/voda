# HOME WATER CONTROLLER — DEV KIT 0.85

Datum: 2026-11-04

## CredentialStore: ověření výsledku commit zápisu

- Opravena nejednoznačnost při zápisu přepínače `cred_active`.
- Dříve `CredentialStore::save()` okamžitě vracel chybu, pokud backend `put()` oznámil neúspěch, i když se hodnota ve skutečnosti mohla dostat do NVS.
- Nově je výsledek commit kroku určen zpětným načtením a přesným porovnáním markeru. Samotný návratový kód `put()` není považován za důkaz, že commit nenastal.
- Pokud zpětné načtení selže nebo vrátí jiný marker, `save()` vrací chybu. Volající musí v takovém případě ponechat AP/web/řídicí cesty zavřené a znovu načíst stav při dalším bezpečném pokusu.
- Přidán fault-injection test, ve kterém backend zapíše marker, ale vrátí `false`; ověřuje se, že read-back správně rozpozná dokončený commit.
- Změna `CredentialStore.h` synchronizována do `firmware/common`, `firmware/ESP32_1/src` a `firmware/ESP32_2/src`.

## Výsledky

- `./tests/run_native_tests.sh`: 47/47 test programů PASS.
- `sh tests/.run_native_tests_asan.sh`: 47/47 test programů PASS.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- ZIP integrity: PASS.

## Omezení

- Změna stále není integrována do boot/AP call-sites obou `.ino` souborů.
- Testy používají hostitelský backend; neprokazují chování fyzické NVS ani Arduino-ESP32 Preferences.
- Provisioning/obnova, odstranění úniků hesel a NVS/Flash Encryption zůstávají otevřené.
- Arduino build ani hardware test nebyl proveden. Peer command path zůstává default-deny a fyzické výstupy neaktivní.
