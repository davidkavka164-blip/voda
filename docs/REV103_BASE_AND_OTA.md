# REV103 – základ pro Windows 7 a úplné OTA sestavy

Výchozí konfigurace vychází z [masteru 3.18 / REV97](../MASTER_HOME_WATER_CONTROLLER_3.18_REV97.md).

## Co nahrát

| Jednotka | První nahrání přes USB v Arduino IDE 1.8.19, core 1.0.6 | Úplná OTA sestava pro tuto jednotku |
| --- | --- | --- |
| ESP32 #1 – studna a dolní nádrž | REV97 `firmware/ESP32_1/ESP32_1.ino` | [ESP32_1_REV103_CORE3_BENCH.bin](../ota/ESP32_1_REV103_CORE3_BENCH.bin) |
| ESP32 #2 – horní nádrž | REV97 `firmware/ESP32_2/ESP32_2.ino` | [ESP32_2_REV102_CORE3_BENCH.bin](../ota/ESP32_2_REV102_CORE3_BENCH.bin) |

[Stáhnout zdrojový základ REV97 pro core 1.0.6](https://github.com/davidkavka164-blip/voda/archive/bb9471ca854d2fb616b9a09db0f7e48d79be0314.zip).
Použij ESP32 Dev Module a výchozí rozdělení flash 4 MB s OTA. OTA soubory
jsou celé aplikační obrazy pro konkrétní jednotku a lze je nahrát přímo
z příslušného ovládacího webu REV97. Na počítači zůstává core 1.0.6.
Jednotka, která již má odpovídající REV97, nepotřebuje znovu nahrát základ.
Nahrání přes OTA nemění tabulku oddílů. Přihlašovací údaje, kalibrace a
nastavení se používají z dosavadního NVS. Firmware je určený pro stolní
zkoušku se samostatně odpojenými výstupy čerpadel a ventilů.

## Co přináší REV103 do ESP32 #1

- Jeden ovládací web pro Wi-Fi, W5500 a servisní AP, s přihlášením a OTA.
- Vypnutí a opětovné zapnutí domácí Wi-Fi z webu bez mazání údajů.
- Servisní AP po 30 sekundách bez domácí Wi-Fi i Ethernetu s IP adresou.
  Po návratu alespoň jednoho připojení servisní AP zhasne.
- Obnovitelné spuštění W5500 s resetem mimo řídicí smyčku. Odkaz zpět na
  hlavní stránku na výsledku OTA. Synchronizace času se může zahájit i
  při funkčním Ethernetu. Synchronizace potřebuje dostupný server NTP.

ESP32 #2 používá stolně ověřenou REV102. Výsledky jsou v
[protokolu ESP32 #2](ESP32_2_CORE3_BENCH_REV98.md).

## Srovnávací zkouška čidla a relé

Přiložený WaterLevelController 2.6.1 slouží jako reference pro funkční
sestavu studny. Používá GPIO34, čidlo 4–20 mA, nominální odpor 150 Ω a
výchozí rozsah 0–10 m. Stejné výchozí hodnoty už má HWC. Referenční projekt
odhaduje napětí z číselné hodnoty ADC. HWC měří kalibrované milivolty a
ověřuje uloženou elektrickou konfiguraci. Porovnává se napětí, proud a
výška při stejné fyzické referenci. Výsledek z referenčního programu se
nezapisuje automaticky do kalibrace nové jednotky.

Referenční projekt používá relé na GPIO5 a vlastní automatiku čerpadla.
Master HWC určuje bezpečnostní výstup studny na GPIO25, výstup dolní nádrže
na GPIO32 a samostatné ventily. Z referenční sestavy lze převzít zkušenost
s ovládáním aktivní úrovní HIGH. Pin, zapojení kontaktů, bezpečný stav při
startu a konfigurace ochran se ověřují pro HWC samostatně.

## Pořadí zkoušky ESP32 #1

1. S odpojenými výstupy nahraj OTA přes Wi-Fi z webu ESP32 #1. Po restartu
   otevři web na ethernetové IP. Ověř spuštění s již zapojeným kabelem.
2. Z ethernetového webu vypni domácí Wi-Fi. Ověř přihlášení a návrat webu
   po odpojení kabelu. Při výpadku obou připojení zkontroluj servisní AP.
   Po návratu Ethernetu ověř, že AP zhasl.
3. Ověř čtení čidla při známé hladině proti referenční sestavě a stav
   ochrany při odpojeném čidle. Relé ověř nejprve bez motorové zátěže.
   Fyzickou zkoušku ventilů a čerpadel řeš samostatně podle masteru.

REV103 zatím čeká na stolní ověření na ESP32 #1.
