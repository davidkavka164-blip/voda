# Audit ukládání domácí Wi-Fi — REV83

Datum: 18. 11. 2026  
Základ: DEV KIT 1.00 REV82

## Rozsah

Audit se týká pouze přihlašovacích údajů domácí Wi-Fi (STA) jednotky ESP32 #1. Nezaměňovat je s přihlašovacími údaji konfiguračního AP a webového administrátora.

## Zjištěný stav ve zdrojích REV82

V `firmware/ESP32_1/ESP32_1.ino`:

- `handleWifi()` otevírá namespace `hwc_wifi` pro zápis.
- SSID zapisuje do klíče `ssid` a heslo samostatně do klíče `password`.
- Návratové hodnoty obou `putString()` se ignorují.
- Následně se obě hodnoty načtou a porovnají s požadavkem; při neshodě se restart neprovede.
- `startWebAndNetwork()` čte oba klíče samostatně a používá je pro `WiFi.begin()`.

Tím je odhalená chyba přesně ohraničená: read-back odhalí některé neúspěšné zápisy při běžícím programu, ale dvojice nemá společný commit marker ani dvě generačně oddělené kopie. Výpadek napájení mezi zápisy může zanechat kombinaci nového SSID a starého hesla (nebo naopak). Následné ověření při ukládání už po restartu neběží.

## Co je již dvouslotové — jiná skupina údajů

`CredentialStore` a `NvsCredentialBackend` obsluhují AP jméno, AP heslo a administrátorské heslo. Používají sloty `c0_*` / `c1_*` a přepínač `cred_active`. Tyto údaje nejsou domácí STA SSID/heslo a tento mechanismus se v REV82 na klíče `hwc_wifi/ssid` a `hwc_wifi/password` nevztahuje.

## Dopad a bezpečnost

- Při přerušení zápisu může dojít ke ztrátě konektivity domácí Wi-Fi a spuštění konfiguračního AP.
- Z dostupného kódu nevyplývá, že by tato chyba mohla přímo změnit GPIO, povolit čerpadlo, otevřít ventil nebo obejít ochranu čerpadel.
- Nejde o důkaz fyzického poškození NVS ani o reprodukovaný brownout; jde o nedostatečnou transakční strukturu dvou samostatných klíčů.
- Stávající plaintext NVS není šifrováním. Tento audit nemění bezpečnostní tvrzení o důvěrnosti uložených údajů.

## Doporučená oprava — zatím neimplementována

1. Přidat samostatné úložiště STA Wi-Fi dvojice se dvěma sloty a generačním/aktivním markerem.
2. Do neaktivního slotu uložit SSID i heslo, zkontrolovat délky a provést přesný read-back obou hodnot.
3. Teprve po ověření obou polí přepnout commit marker; po restartu načítat pouze potvrzený slot.
4. Zachovat kompatibilní migraci dosavadních klíčů `ssid` a `password`. Staré klíče nemažat, dokud nový slot není plně ověřen a potvrzen.
5. Při poškozeném nebo nejednoznačném markeru nesmí firmware sestavit smíšenou dvojici. Má bezpečně přejít do stávajícího konfiguračního AP/recovery režimu.
6. Otestovat přerušení po každém jednotlivém zápisu, před/po read-backu a při zápisu markeru; simulovat i backend, který hlásí plný zápis, ale vrátí starý obsah.

Před implementací je nutné ověřit přesnou verzi Arduino-ESP32/ESP-IDF a kompatibilitu NVS API. Migrace musí zachovat současné údaje a nesmí automaticky mazat původní klíče.

## Rozhodnutí REV83

REV83 je auditní revize. Firmware se nemění. Oprava je připravena jako následující samostatná implementační práce, aby nebylo riskováno neotestované přepisování uložených přístupových údajů.
