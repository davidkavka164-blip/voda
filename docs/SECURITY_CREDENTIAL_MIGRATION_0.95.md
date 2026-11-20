# Bezpečná migrace credentials do jednotného úložiště — revize 0.95

## Účel

Předchozí revize měla dva různé způsoby ukládání přístupových údajů:

- ESP32 #1: AP heslo v `hwc_ap/password`, administrátorské heslo v `hwc_auth/admin`, SSID odvozené z MAC.
- ESP32 #2: SSID, AP heslo a administrátorské heslo v `hwc_u2ui`.

Od 0.95 je autoritativní úložiště na obou jednotkách `hwcCred` s dvouslotovým `CredentialStore`.

## Rozhodovací tabulka

| Stav `hwcCred` | Chování při bootu |
|---|---|
| `VALID` | Načíst kompletní aktivní sadu výhradně z `hwcCred`; legacy namespace nečíst |
| `EMPTY` | Pokusit se o jednorázovou migraci kompletní a validní legacy sady |
| `CORRUPT` | Fail-closed; AP/web se nesmí spustit a legacy fallback je zakázán |
| `IO_ERROR` | Fail-closed; AP/web se nesmí spustit a legacy fallback je zakázán |

Toto rozlišení je zásadní. Návrat ke starému namespace při poškození nového selectoru by mohl znovu povolit přístup s již překonanou nebo neúplnou sadou údajů.

## Pořadí migrace

1. Otevřít `hwcCred` a načíst stav `CredentialStore`.
2. Při `VALID` použít pouze aktivní slot a skončit.
3. Při `CORRUPT`/`IO_ERROR` skončit bez fallbacku.
4. Pouze při `EMPTY` otevřít staré namespace a načíst legacy údaje přes `CredentialStartupGate`, který odmítne poškozený existující klíč a ověří zpětné načtení nově vytvořeného údaje.
5. Sestavit pevně dimenzovaný `Credentials` buffer a ověřit jej přes `CredentialStore::validate()`, která kontroluje délky i tisknutelné ASCII bez řídicích/non-ASCII znaků.
6. `CredentialLegacyMigrator` znovu ověří, že cíl je `EMPTY`; poté použije privátní dvouslotový staging/commit.
7. Načte celý store znovu a vyžaduje přesnou shodu všech tří hodnot.
8. Teprve po výsledku `CREDENTIAL_MIGRATION_DONE` převezme firmware ověřená data a případně provede jednorázové předání nově vytvořených hesel.
9. Při jakémkoli neúspěchu se AP/web credentials v RAM vyprázdní a AP/web zůstane vypnutý.

## Vlastnosti a záruky

- Migrace nepřepisuje `VALID`, `CORRUPT` ani `IO_ERROR` cíl.
- Migrace se nepokouší opravovat poškozený nový store pomocí legacy namespace.
- Staré klíče se nemažou, aby tento přechodný krok nezvyšoval riziko ztráty dat. Nejsou však fallback cestou.
- Zápis probíhá přes stávající dvouslotovou logiku; selector se aktivuje až po ověření kompletního slotu.
- Opakovaný boot po úspěšné migraci načítá jen nový store a znovu nevypisuje hesla.
- Neúspěšná migrace nechává nový store prázdný nebo rozpoznatelně poškozený; v obou případech zůstává AP blokovaný.

## Co tato revize neřeší

- NVS/Flash Encryption není zapnuto; hodnoty v `hwcCred` nejsou kryptograficky chráněné proti čtení flash.
- USB/UART fyzická autorizace a provisioning board adapter nejsou připojeny do `.ino`.
- Jednorázový Serial handoff stále existuje pouze při úspěšné migraci nově vygenerovaných hodnot.
- Změna/rotace hesel z dashboardu zatím není integrovaná do `CredentialStore`.
- Chování skutečné NVS při přerušení napájení a build Arduino-ESP32 musí být ověřeny na cílovém hardware.
