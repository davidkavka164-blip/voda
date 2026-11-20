# HOME WATER CONTROLLER — DEV KIT 1.00 REV83

Datum: 18. 11. 2026  
Základ: DEV KIT 1.00 REV82

## Změna

REV83 je auditní revize ukládání domácích Wi-Fi STA údajů jednotky ESP32 #1.

Bylo ověřeno, že `handleWifi()` zapisuje SSID a heslo samostatně do `hwc_wifi/ssid` a `hwc_wifi/password`. Následný přesný read-back zachytí neúspěšné uložení při běhu, ale nezajišťuje transakční konzistenci při výpadku napájení mezi zápisy.

Dvouslotový `CredentialStore` se vztahuje pouze na konfigurační AP a webové administrátorské údaje (`c0_*`, `c1_*`, `cred_active`), nikoli na domácí Wi-Fi STA dvojici.

## Rozhodnutí

- Firmware se v REV83 nemění.
- Je zdokumentován návrh dvouslotového STA úložiště s commit markerem, úplným read-backem a bezpečnou migrací původních klíčů.
- Původní klíče se při budoucí migraci nesmí mazat před potvrzením nového slotu.
- Detailní audit: `docs/WIFI_CREDENTIAL_STORAGE_AUDIT_REV83.md`.

## Ověření

Protože REV83 nemění zdrojový kód firmware, regresní sady REV82 zůstávají referenční. Před vydáním byly znovu spuštěny nativní a Python testy a kontrola syntaxe skriptů. Arduino cílový překlad ani fyzické NVS/power-cut testy nebyly provedeny.
