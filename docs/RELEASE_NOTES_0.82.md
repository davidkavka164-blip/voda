# HOME WATER CONTROLLER DEV KIT 0.82

Datum: 2026-11-04

## Rozsah

Auditní revize bez změny firmware. Doplněn `SECURITY_CREDENTIAL_BOOT_ORDER_AUDIT_0.82.md` a stav implementačního plánu.

## Hlavní výsledek

Potvrzeno, že ESP32 #1 může pokračovat se spuštěním AP po nedostupnosti NVS nebo neúspěšném uložení AP hesla. ESP32 #2 nekontroluje návratové hodnoty zápisů ani read-back před spuštěním AP a web serveru. U obou jednotek zůstává vystavení tajemství v Serial logu; ESP32 #1 navíc zobrazuje AP heslo v HTML.

Nebyla provedena izolovaná oprava fail-closed, protože bez dokončeného fyzicky autorizovaného provisioningu by zablokovala nové jednotky a mohla znemožnit prvotní konfiguraci. Další krok je implementace a testování CredentialStore/staging-commit před úpravou call-sites.

## Ověření

Stávající testy se mají spustit při zabalení revize. Arduino build, hardware, NVS/Flash Encryption a power-cut testy nejsou provedeny. Peer command path zůstává default-deny a fyzické výstupy neaktivní.
