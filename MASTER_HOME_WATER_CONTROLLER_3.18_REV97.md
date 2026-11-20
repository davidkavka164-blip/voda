# HOME WATER CONTROLLER – MASTER 3.18 / REV97

Datum: 20. 11. 2026

Tento dokument aktualizuje stav sítě a ověření [masteru 3.17 / REV94](MASTER_HOME_WATER_CONTROLLER_3.17_REV94.md). Zásady ochrany, zapojení a řízení vody z masteru 3.17 platí dál.

## Síť a aktuální firmware

- Cílové sestavení je ESP32 Dev Module, Arduino-ESP32 1.0.6, Ethernet 2.0.2 a výchozí rozdělení flash 4 MB s OTA. Experiment REV94 s EthernetESP32 pro core 3.x je samostatný.
- Na obou jednotkách zajišťuje stávající `WebServer` ovládání a OTA přes domácí Wi-Fi nebo servisní AP. W5500 přes klasické `Ethernet.h` poskytuje pouze přihlašovaný přehled `/status`. Sdílený modul používá stejné heslo správce jako Wi-Fi a vlastní omezení pokusů o přihlášení. Rozšíření ovládání na Ethernet zůstává mimo tuto etapu.
- W5500 má na obou jednotkách CS 27, RESET 26, SCK 18, MISO 19, MOSI 23. DHCP a obsluha ethernetových klientů běží v samostatné úloze. Řídicí smyčka předává přehledu kopii měření.
- REV97 rozšířila omezené HTTP hlavičky a rozlišuje chybu velikosti a nedokončený požadavek. Hláška „nedostupné“ značí chybějící platné čerstvé měření, například na stolní sestavě bez kalibrovaných hladinových čidel.
- Jednotky zůstávají nezávislé bez vzájemné komunikace. Síťové chyby nesmějí povolit čerpadlo ani otevřít ventil. Zůstává stávající ověřování správce, Origin/Host v ovládacím webu, limity přihlášení, OTA blokace a místní bezpečnostní logika.

## Důkazy o ověření

| Oblast | Stav |
|---|---|
| ESP32 #1 – servisní AP a ovládací web z domácí Wi-Fi | Uživatel ověřil přístup na desce. |
| ESP32 #1 – Ethernet v REV97 | Uživatel ověřil přihlášený `/status` i obnovení stránky po vytažení a zapojení kabelu bez restartu. |
| ESP32 #1 – aktualizace OTA | K dispozici je sestavený a ověřený aplikační soubor. Z dosavadních zpráv není jisté, zda byl REV97 nahrán přes OTA nebo USB. |
| ESP32 #2 – AP, Wi-Fi STA, W5500, OTA | Sestavení prošlo. Souběh AP a STA, přihlášení přes W5500, návrat spojení a OTA ještě neprošly potvrzenou zkouškou na desce. |
| Software REV97 | Oba sketchy přeloženy s core 1.0.6. Prošlo 81 nativních testovacích programů, statické kontroly a 17 testů v Pythonu. |
| Čidla, relé, ventily a čerpadla | Provozní ověření zůstává otevřené. Úspěšná síťová zkouška nepotvrzuje elektrické zapojení ani funkci ochran na konkrétní instalaci. |

## Další stolní zkouška

U ESP32 #2 s odpojenými výstupy čerpadla nejprve nahraj odpovídající sketch přes USB, pokud na desce ještě není její firmware. Ověř servisní AP a přihlášení, ulož údaje domácí Wi-Fi a ověř ovládací web přes domácí IP i servisní AP. Potom připoj W5500, otevři `/status` na jeho IP a zkus odpojit a vrátit síťový kabel bez restartu. Přehled má zobrazit horní hladinu a teplotu nebo příslušnou informaci o nedostupnosti. K případné zkoušce OTA použij aplikační soubor výhradně pro danou jednotku.

- [Aplikační soubor REV97 pro ESP32 #1](https://raw.githubusercontent.com/davidkavka164-blip/voda/ota-rev97/ESP32_1_REV97.bin)
- [Aplikační soubor REV97 pro ESP32 #2](https://raw.githubusercontent.com/davidkavka164-blip/voda/ota-rev97/ESP32_2_REV97.bin)
