# DEV KIT 1.00 AUDIT — REV13

Datum: 2026-11-08

## Účel revize

Implementace jedné úzce vymezené opravy z auditu REV12: rychlý restart ESP32 #2 během aktivního transferu horní → dolní už nesmí umožnit pokračování původního požadavku pod novou peer boot epochou.

## Změna

- `PeerStatusMonitor` nově zaznamená přijetí první platné zprávy s novým `bootId` jako jednorázovou událost `consumeBootEpochChanged()`.
- První objevení peeru po startu monitoru není hlášeno jako restart; událost vzniká pouze při změně již známého `bootId`.
- ESP32 #1 při této události zkontroluje, zda je aktivní transfer horní → dolní ve stavu PRECHECK nebo RUNNING. Pokud ano, ukončí jej přes `TRANSFER_REASON_COMMUNICATION`.
- Výstupní logika transferu následně uloží výsledek přes stávající historii/transfer journal. Příchozí čerstvá data z ESP32 #2 původní požadavek neobnoví; uživatel musí zadat nový požadavek.
- Událost se spotřebuje pouze jednou. Duplicitní nebo opožděný rámec z odmítnuté staré epochy novou událost nevyvolá.
- `PeerProtocol.h/.cpp` zůstávají byte-identické v `firmware/common`, `firmware/ESP32_1/src` a `firmware/ESP32_2/src`.

## Co se nemění

- Žádné hodnoty ani významy limitů horní nádrže.
- Oddělení kritické hladiny ochrany čerpadla, obnovovací hladiny ochrany, rezervy objektu a prahu otevření ventilu.
- `pumpProtectionReady` zůstává diagnostický údaj; není z něj vytvořena nová podmínka transferu.
- Nouzové ruční ovládání ventilů se touto změnou neimplementuje ani nemění.
- Fyzické výstupy zůstávají vypnuté; žádný GPIO ani fyzický ventil nebyl aktivován.

## Testy

- Nativní C++: 59/59 PASS.
- ASan/UBSan: 59/59 PASS.
- Statická integrace `check_peer_boot_epoch_transfer_abort.sh`: PASS.
- Synchronizace společných modulů pro obě ESP32: PASS.
- Python unittest: 17/17 PASS.
- `compileall`: PASS.
- Arduino-ESP32 build a fyzický test W5500/ESP32/NVS/ventilů nebyly provedeny.

## Bezpečnostní poznámka

Jde o opravu logiky restartu peeru v simulačním/řídicím toku. Není to důkaz funkčnosti fyzického uzavření ventilu, protože fyzické výstupy zůstávají záměrně vypnuté.
