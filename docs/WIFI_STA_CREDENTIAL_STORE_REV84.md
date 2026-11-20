# Transakční ukládání domácí Wi-Fi STA — REV84

## Úložiště

Namespace: `hwc_wifi` (stejný jako dosavadní domácí Wi-Fi údaje).

| Účel | Klíče |
|---|---|
| Slot 0 | `s0_ssid`, `s0_pass` |
| Slot 1 | `s1_ssid`, `s1_pass` |
| Potvrzený slot | `sta_active` (`0` nebo `1`) |
| Legacy migrace | `ssid`, `password` |

## Zápis

1. Načte se aktivní selector.
2. Vybere se opačný slot (při prvním zápisu slot 0).
3. SSID i heslo se zapíší do cílového slotu.
4. Každé pole se přesně načte zpět; následně se načte a porovná celá dvojice.
5. Teprve poté se zapíše `sta_active` a ověří se selector i celý vybraný slot.

Předchozí aktivní slot se nemaže ani neupravuje. Nedokončený kandidát bez potvrzeného selectoru se nepoužije. Opakovaný pokus může bezpečně přepsat slot 0, pokud selector dosud neexistuje.

## Migrace

Pokud `sta_active` chybí, čte se legacy dvojice `ssid` a `password`. Migrace je možná pouze tehdy, jsou-li dostupné obě hodnoty a SSID má 1–32 bajtů a heslo 0–64 bajtů. Prázdné heslo je povoleno pro otevřenou Wi-Fi síť.

Legacy klíče se nemažou. Pokud se zápis přeruší, další start může migraci opakovat z původní dvojice. Pokud je legacy dvojice neúplná/poškozená nebo existuje osiřelý slot bez legacy zdroje, stav je odmítnut a zařízení použije konfigurační AP.

## Ověření a omezení

Hostitelský test pokrývá selhání zápisu do neaktivního slotu, zachování předchozí aktivní dvojice, opakování přerušené migrace, backend s falešně úspěšným zápisem, otevřenou síť a poškozený selector. Jde o deterministický model aplikační logiky; neprokazuje fyzickou atomitu NVS při brownoutu. Před nasazením je nutný Arduino build s přesným ESP32 core a fyzický test výpadku napájení.
