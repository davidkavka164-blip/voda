# NVS žurnál horních limitů — REV78

Datum: 16. 11. 2026 | Základ: DEV KIT 1.00 REV77

## Rozsah

Změna se týká `NvsUpperTankLimitsStore` v namespace `hwc_upperlimits`. Veřejné API `begin/end/hasRecord/load/save` zůstává beze změny. Ukládají se pouze nastavitelné limity horní nádrže. Kritická západka horního čerpadla zůstává v samostatném `NvsUpperTankProtectionStore`.

## Formát

- `slot0`, `slot1`: střídavé záznamy limitů s verzí, generací a CRC.
- `head`: ověřený ukazatel na potvrzený slot a jeho generaci.
- `txn`: transakční záznam s předchozím potvrzeným slotem a kandidátním slotem.
- `state`: původní REV77 blob, zachovaný jako migrační zdroj.

CRC se počítá ze sémantických polí, nikoli z compiler paddingu. Každý slot i metadata se po zápisu zpětně načtou a ověří.

## Pořadí zápisu

1. Zapsat a ověřit `txn`.
2. Zapsat kandidáta do neaktivního slotu.
3. Zpětně načíst kandidáta a ověřit generaci, CRC a všechna pole.
4. Zapsat a ověřit `head`.
5. Odstranit `txn`.

Předchozí potvrzený slot se během zápisu kandidáta nepřepisuje.

## Obnova po restartu

- Platný `txn` a úplný cílový slot: dokončí se commit nového záznamu.
- Platný `txn`, neúplný cílový slot a platný předchozí slot: obnoví se předchozí potvrzená konfigurace.
- Platný `head` bez transakce: načte se pouze slot, na který ukazuje.
- Poškozený aktivní slot nebo `head`: fail-closed. Poškozený `txn` lze ignorovat pouze tehdy, pokud existuje platný `head` a přesně odpovídající aktivní slot; protože marker se zapisuje před kandidátem, potvrzovací `head` určuje poslední commit. Bez platného `head` zůstává stav fail-closed.
- Poškozený neaktivní slot bez otevřené transakce neblokuje validní aktivní slot; při dalším zápisu se nahradí.
- Přetečení generace na nulu se odmítá.
- Při přerušené úplně první inicializaci není stará konfigurace k dispozici. Pumpa zůstává blokovaná; nový výslovný `save()` může dokončit pouze tuto počáteční transakci.

## Migrace původního `state`

Původní záznam se validuje přes přesnou délku, verzi, rozsahy a původní CRC. Před vytvořením `slot0` se zapíše migrační `txn`; následně se ověří `slot0` a zapíše `head`. Původní `state` se nemaže ani nepřepisuje. Pokud se migrace přeruší během zápisu slotu, validní `state` zůstává a další start může migraci zopakovat. Poškozený `state`, osiřelé sloty bez metadat nebo neplatná metadata se nepovažují za čistou instalaci; automatické výchozí hodnoty ani mazání se neprovádějí.

## Bezpečnostní hranice

- Změna nemění kritickou západku, elektrickou kalibraci snímače ani objemovou kalibraci.
- Chyba načtení limitů stále znamená neplatnou konfiguraci a blokaci horního čerpadla.
- Nouzové povolení čerpadla není opravou NVS.
- Žádné piny, zapojení, relé, ventily ani fyzické limity se nemění.

## Ověření a omezení

Hostitelský test pokrývá počáteční zápis, střídání slotů, výpadek při zápisu kandidáta, návrat na předchozí potvrzený záznam, výpadek při potvrzení `head`, explicitní opakování první inicializace, migraci původního `state`, opakování přerušené migrace a poškození aktivního/neaktivního záznamu a přerušený zápis transakčního markeru.

Test používá model `Preferences` s injekcí částečného zápisu. Neprokazuje fyzické chování konkrétní verze ESP32 NVS při brownoutu. Arduino cílový překlad a fyzický test na ESP32 #2 jsou stále nutné.
