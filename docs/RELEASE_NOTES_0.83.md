# HOME WATER CONTROLLER — DEV KIT 0.83

Datum: 2026-11-04

## CredentialStore — přenositelné jádro

- Přidán `firmware/common/CredentialStore.h` jako hostitelsky testovatelná politika pro trojici AP SSID, AP heslo a administrátorské heslo.
- Zápis používá dva sloty: data neaktivního slotu se zapíší, každá položka se zpětně načte a ověří, celá sada se znovu validuje a až poté se přepne `cred_active`.
- Při selhání stagingu/read-back se neaktivní slot invaliduje; aktivní slot zůstává nedotčen.
- `load()` rozlišuje `VALID`, `EMPTY`, `CORRUPT` a `IO_ERROR`. Částečná sada bez commit markeru není považována za prázdné úložiště.
- Backend má výslovně rozlišovat chybějící klíč od I/O chyby. `erase()` musí být bezpečné pro invalidaci slotu.
- Přidán fault-injection test `test_credential_store.cpp`.

## Omezení / neuzavřené body

Toto je pouze přenositelné jádro a kontrakt backendu. Není dodán produkční `Preferences`/NVS adapter, není ověřena NVS/Flash Encryption a `CredentialStore` zatím není zapojen do `ESP32_1.ino` ani `ESP32_2.ino`. AP call-sites, Serial výpisy a HTML zobrazení hesla proto zůstávají beze změny. Samotný staging/commit v běžné NVS není šifrování a neposkytuje ochranu proti fyzickému čtení flash.

Peer příkazy zůstávají default-deny; fyzické výstupy nejsou aktivovány.
