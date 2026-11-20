# Release notes — DEV KIT 0.79

Datum: 2026-11-04

## Rozsah

Dokumentační bezpečnostní revize navazující na audit přihlašovacích údajů 0.78.

- Byla ověřena struktura dodaného balíčku: síťová/AP/webová implementace není v aktuálním firmware stromu obsažena.
- Nebyl vytvořen izolovaný NVS helper, který by nebylo možné napojit na skutečný start AP.
- Doplněna integrační brána definující stavové výsledky, fail-closed chování, požadavky na read-back a testovací matici.
- Výslovně odděleno ověření integrity zápisu od šifrování tajemství.

## Testy

Bez změny zdrojového kódu. Před vydáním balíčku byly znovu spuštěny standardní a ASan/UBSan nativní testy, boot identity, Ethernet recovery a kontrola archivu.

## Omezení

P0-4 až P0-6 zůstávají otevřené. Skutečná integrace vyžaduje síťové sketch soubory obou jednotek. Arduino build, NVS Encryption, Flash Encryption a hardware nebyly ověřeny. Peer command path zůstává default-deny; fyzické výstupy neaktivní.
