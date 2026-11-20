# Release notes — DEV KIT 0.77

Datum: 2026-11-04

## Bezpečnostní návrh přístupových údajů
- Přidán `SECURITY_CREDENTIAL_PROVISIONING_DESIGN_0.77.md`.
- Definován životní cyklus prvotního nastavení, běžné změny a fyzicky potvrzené obnovy AP/admin hesel pro obě ESP32.
- Stanoveno, že hesla se nikdy nezobrazují v dashboardu ani nevypisují do Serial Monitoru; staré heslo nelze načíst zpět.
- Definovány požadavky na časově omezený servisní režim, fail-closed NVS chyby a regresní testy.
- GPIO servisního vstupu nebylo přiděleno: typ/revize desek a volné piny nejsou potvrzeny.

## Rozsah změny
- Dokumentační revize. Firmware beze změny; nálezy P0-4 až P0-6 zůstávají otevřené.
- Standardní a ASan/UBSan testy se mají spustit znovu při sestavení balíčku; Arduino build ani hardware nebyly ověřeny.

## Kontrola balíčku
- Standardní nativní testy: 45/45 PASS.
- ASan/UBSan: 45/45 PASS.
- `check_peer_boot_identity.sh`: PASS.
- `check_peer_ethernet_recovery.sh`: PASS.
- Arduino-ESP32 build a fyzický hardware test nebyly provedeny.
