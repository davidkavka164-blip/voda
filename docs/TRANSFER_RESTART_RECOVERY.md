# Obnova transferu po restartu — revize 0.66

## Účel

Zajistit, aby se po restartu ESP32 #1 nikdy automaticky neobnovil požadavek na transfer ani se znovu nepovolilo otevření ventilu. Mechanismus je nyní připojen k webové SIMULACI; fyzické ventily a čerpadla zůstávají odpojené od aplikační logiky.

## Trvalý žurnál

`TransferRestartGuard` používá rozhraní `ITransferJournalStore`. ESP32 adaptér `NvsTransferJournalStore` ukládá záznam do dvou NVS slotů `slot0` a `slot1` v namespace `hwc_xfer_jrnl`.

Každý záznam obsahuje verzi, stav, zdroj, request ID, požadovaný objem, počáteční a cílovou hladinu, odhad výsledku, koncový stav/důvod, generační čítač a CRC32. Zápis probíhá do neaktivního slotu; po zápisu se znovu načte a ověří. Starší platný slot zůstává zachován jako fallback.

CRC32 detekuje náhodné poškození, nikoli úmyslnou manipulaci ani rollback flash. Dvouslotový návrh omezuje riziko ztráty posledního platného záznamu, ale skutečné chování při výpadku napájení musí být ověřeno na cílovém ESP32 a přesné verzi ESP-IDF/Arduino core.

## Stavy a pravidla

| Stav žurnálu | Význam | Chování po restartu |
|---|---|---|
| CLEAR | Není otevřený transfer | Povolí přípravu nového požadavku |
| PREPARED | Požadavek přijat, příprava probíhá | Změní se na INTERRUPTED; nic se neobnovuje |
| ACTIVE | Transferový automat přešel do běhu | Změní se na INTERRUPTED; nic se neobnovuje |
| INTERRUPTED | Předchozí transfer nebyl čistě uzavřen | Blokuje nové transfery do potvrzení obsluhou |
| RESULT_PENDING | Automat má koncový výsledek, ale výsledek ještě není potvrzen v historii | Blokuje nové transfery; výsledek lze rekonstruovat z žurnálu |

`canOpenValve()` vrací true pouze v běhovém stavu ACTIVE. Při chybě zápisu, zpětného načtení, poškození nebo nedostupnosti NVS se guard uzamkne do fail-closed stavu `TRANSFER_RESTART_LOCKED`.

## Průběh v ESP32 #1

1. Webový požadavek je validován; spuštění vyžaduje dostupný žurnál i historii NVS. Před předáním automatu se uloží PREPARED.
2. Při přechodu `TransferLogic` do RUNNING se uloží ACTIVE. Pokud zápis selže, automat se násilně ukončí a vrátí `VALVES_CLOSE_ALL`.
3. Při koncovém stavu se uloží RESULT_PENDING včetně výsledku.
4. Výsledek se uloží do historie transferů. Teprve po úspěšném uložení historie se žurnál vymaže.
5. Po restartu ve stavu INTERRUPTED nebo RESULT_PENDING web zobrazí varování a nabídne explicitní potvrzení. Potvrzení nejprve zajistí záznam v historii, potom vymaže žurnál.

Přerušení PREPARED/ACTIVE se do historie zapisuje jako ABORTED / SYSTEM_NOT_READY. Odhad přečerpaného objemu je při takovém restartu nula, protože bez kontinuálního měření nelze bezpečně určit, kolik vody se případně skutečně přemístilo. Nejde o potvrzení fyzického průtoku.

## Testy

- restart z PREPARED → INTERRUPTED;
- restart z ACTIVE → INTERRUPTED a zákaz otevření ventilu;
- restart z RESULT_PENDING → blokace do potvrzení;
- explicitní potvrzení a uvolnění;
- selhání zápisu → LOCKED;
- poškozený záznam → LOCKED;
- dvouslotový fallback na starší platný záznam;
- skutečná kombinace guard + NVS adaptér ve stubovaném hostitelském prostředí.

## Omezení

- Implementace žurnálu je zapojena do transferové SIMULACE ESP32 #1. Výkonové GPIO nejsou řízena a fyzické ventily/čerpadla zůstávají zakázané.
- `canOpenValve()` je bezpečnostní podmínka pro budoucí výstupní vrstvu; před fyzickým řízením musí být prokázáno, že každý jednotlivý výstupní zápis je tímto povolením skutečně podmíněn.
- Arduino-ESP32 build nebyl proveden. Skutečné NVS, výpadek napájení, reset během zápisu a chování ventilů nebyly fyzicky testovány.
- Při nedostupné historii se potvrzení obnovy odmítne; obsluha musí počkat na obnovení úložiště. Není dovoleno přepsat historii neúplným RAM snapshotem.
