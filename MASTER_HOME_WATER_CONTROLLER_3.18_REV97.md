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
| ESP32 #1 – aktualizace OTA | Uživatel potvrdil úspěšné nahrání REV97 přes OTA a po aktualizaci ověřil ethernetový přehled. |
| ESP32 #2 – Ethernet | Uživatel potvrdil úspěšnou stolní zkoušku W5500. Návrat spojení po odpojení kabelu nebyl výslovně potvrzen. |
| ESP32 #2 – Wi-Fi a OTA | Přístup přes servisní AP byl při dosavadní stolní zkoušce funkční. Přístup přes domácí Wi-Fi, souběh se servisním AP a OTA čekají na samostatné potvrzení. |
| Software REV97 | Oba sketchy přeloženy s core 1.0.6. Prošlo 81 nativních testovacích programů, statické kontroly a 17 testů v Pythonu. |
| Čidla, relé, ventily a čerpadla | Provozní ověření zůstává otevřené. Úspěšná síťová zkouška nepotvrzuje elektrické zapojení ani funkci ochran na konkrétní instalaci. |

## Navazující stolní ověření ESP32 #2

Na samostatné větvi `esp32-2-core3-bench` uživatel potvrdil plný ovládací
web přes Wi-Fi i Ethernet, OTA přes Ethernet bez domácí Wi-Fi a následně
funkční stolní zkoušku REV102 s vypínáním domácí Wi-Fi a záložním servisním
AP. Výsledky jsou vedené v [protokolu stolní zkoušky](https://github.com/davidkavka164-blip/voda/blob/a382fc45d5df50c58708087a6ead1f319e667f4b/docs/ESP32_2_CORE3_BENCH_REV98.md).
Firmware ve větvi `main` zůstává REV97. Provozní ověření čidel, relé a
ventilů zůstává otevřené.

## Základ pro core 1.0.6 a úplné OTA sestavy

REV97 zůstává základem pro první nahrání obou jednotek v Arduino IDE na
Windows 7. Na zkušební větvi je úplná OTA sestava REV103 pro ESP32 #1
a ověřená REV102 pro ESP32 #2. REV103 přenáší síťové funkce ESP32 #2
na ESP32 #1 a čeká na její samostatnou stolní zkoušku. Srovnávací použití
přiloženého projektu čidla a relé je popsané v [postupu REV103](https://github.com/davidkavka164-blip/voda/blob/3cf829d09baca359cdc46344a3c64fe256d27702/docs/REV103_BASE_AND_OTA.md).

## Web samostatných jednotek v REV104

Na zkušební větvi REV104 popisuje ESP32 #1 místní řízení přepouštění a
kontrolu přítoku do dolní nádrže. ESP32 #2 zobrazuje teplotu a hladinu pro
ochranu horního čerpadla, se třemi místními mezemi ochrany. Z webu jsou
vyřazeny historické funkce zásoby pro domácnost a objemové kalibrace
horní nádrže. Uložené objemové údaje se nemažou. Oba OTA obrazy jsou
popsané v [REV104](https://github.com/davidkavka164-blip/voda/blob/d01f805a6c1a6da4e4b8c6d3d89a1ae41e247120/docs/REV104_LOCAL_WEB.md).
REV97 dál slouží pro první nahrání s core 1.0.6.

## Další stolní zkouška v REV97

U ESP32 #2 s odpojenými výstupy čerpadla nejprve nahraj odpovídající sketch přes USB, pokud na desce ještě není její firmware. Ověř servisní AP a přihlášení, ulož údaje domácí Wi-Fi a ověř ovládací web přes domácí IP i servisní AP. U již ověřeného W5500 zbývá potvrdit obnovu spojení. Otevři `/status`, odpoj a připoj síťový kabel a obnov stránku bez restartu. Přehled má zobrazit horní hladinu a teplotu nebo příslušnou informaci o nedostupnosti. K případné zkoušce OTA použij aplikační soubor výhradně pro danou jednotku.

- [Aplikační soubor REV97 pro ESP32 #1](https://raw.githubusercontent.com/davidkavka164-blip/voda/ota-rev97/ESP32_1_REV97.bin)
- [Aplikační soubor REV97 pro ESP32 #2](https://raw.githubusercontent.com/davidkavka164-blip/voda/ota-rev97/ESP32_2_REV97.bin)
