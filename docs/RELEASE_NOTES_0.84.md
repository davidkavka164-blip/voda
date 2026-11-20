# HOME WATER CONTROLLER — DEV KIT 0.84

Datum: 2026-11-04

## Preferences/NVS backend pro CredentialStore

- Přidán header-only `NvsCredentialBackend.h` pro Arduino-ESP32 `Preferences`.
- Backend používá samostatný namespace `hwcCred` a byte-string operace `putBytes/getBytes/getBytesLength`, aby mohl kontrolovat přesnou délku včetně ukončovacího NUL.
- `get()` rozlišuje chybějící klíč od chyby čtení, odmítá prázdný, příliš dlouhý, neukončený i vnitřním NUL poškozený payload.
- `put()` ověřuje počet zapsaných bajtů; vyšší `CredentialStore` následně provádí zpětné načtení a porovnání.
- `erase()` je idempotentní pro chybějící klíč a kontroluje výsledek `Preferences.remove()`.
- Stejný soubor je synchronizován do `firmware/common`, `ESP32_1/src` a `ESP32_2/src`.
- Přidán hostitelský test NVS adapteru; standardní i ASan/UBSan suite nyní 47/47.

## Omezení

- Hostitelský test používá Preferences stub; není to důkaz překladu proti konkrétní Arduino-ESP32/Preferences knihovně ani chování fyzické NVS.
- Adapter zatím není zapojen do boot/AP call-sites obou `.ino` souborů.
- Nejsou odstraněny Serial/HTML úniky hesel; provisioning a fyzická obnova nejsou uzavřeny.
- NVS/Flash Encryption není ověřeno. Samotný Preferences namespace ani byte storage neposkytuje šifrování.
- Peer command path zůstává default-deny; fyzické výstupy nejsou aktivovány.
