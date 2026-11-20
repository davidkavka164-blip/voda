# HOME WATER CONTROLLER — integrační brána bezpečného úložiště přístupových údajů

Revize: 0.79  
Stav: SUPERSEDED revizí 0.80.

**Oprava:** Původní tvrzení, že v balíčku chybí skutečné AP/Wi-Fi/web call-sites obou jednotek, bylo chybné. Oba hlavní soubory `firmware/ESP32_1/ESP32_1.ino` a `firmware/ESP32_2/ESP32_2.ino` tyto call-sites obsahují. Viz `SECURITY_CREDENTIAL_CALLSITE_AUDIT_0.80.md`, který uvádí přesné řádky a bezpečnostní nálezy.

Původní návrh rozhraní CredentialStore a požadavek na fail-closed chování zůstávají užitečné, ale integrační blokátor „chybí síťové sketch soubory“ se ruší. Implementace musí vycházet ze skutečných call-sites a nesmí být vydávána za hotovou, dokud není zapojena do obou startů AP a ověřena regresními testy.
