# REV104 – web podle samostatné role jednotek

Navazuje na [základ REV97 a souhrnné OTA sestavy](REV103_BASE_AND_OTA.md)
a [master 3.18](../MASTER_HOME_WATER_CONTROLLER_3.18_REV97.md).

- **ESP32 #1** spravuje studnu, dolní nádrž a ventily. Web vysvětluje, že při
  přepouštění z horní nádrže sleduje přírůstek v dolní nádrži. Ochrana
  horního čerpadla je místní na ESP32 #2.
- **ESP32 #2** zobrazuje teplotu vody, hladinu potřebnou pro ochranu, stav
  ochrany a bezpečnostního relé. Nastavení umožňuje zadat využitelnou
  hloubku, kritickou a obnovovací hladinu. Zůstává servisní obnova
  elektrické kalibrace, nouzové ovládání čerpadla, nastavení sítě a OTA.
- Původní formuláře zásoby pro domácnost, objemové kalibrace a související
  webové cesty ESP32 #2 jsou vyřazené. Staré objemové záznamy v NVS se
  nemažou. Formát uložených mezí zůstává kompatibilní. Dřívější pole
  rezervy a přepouštění se zachovají, pokud vyhovují formátu záznamu.
  Při nové nebo nekompatibilní konfiguraci se vyplní kompatibilními
  hodnotami. Místní ochrana čerpadla je nepoužívá.
- Funkční síťové úpravy zůstávají součástí obou sestav. Pro první nahrání
  přes USB na Windows 7 se dál používá zdrojový základ REV97 s core 1.0.6.

Každý soubor OTA je celá aplikace pro danou jednotku. Při stolní zkoušce
nech výstupy čerpadel a ventilů odpojené.

- [ESP32 #1 – OTA REV104](../ota/ESP32_1_REV104_CORE3_BENCH.bin)
- [ESP32 #2 – OTA REV104](../ota/ESP32_2_REV104_CORE3_BENCH.bin)

## REV105 – nákresy hladin

ESP32 #1 zobrazuje schematický průřez studnou a dolní nádrží. ESP32 #2
zobrazuje horní nádrž v části ochrany čerpadla. Modrá vyplněná část odpovídá
změřené výšce proti využitelné hloubce nebo maximu dolní nádrže. Číslo je
v centimetrech. Nákres vyžaduje platný a čerstvý vzorek s ověřeným nastavením
snímače. Zastaralý nebo neplatný vzorek označí jako nedostupný. Pokud chybí
rozsah nebo měření leží mimo něj, číslo nemá modrou výplň.

- [ESP32 #1 – OTA REV105](../ota/ESP32_1_REV105_CORE3_BENCH.bin)
- [ESP32 #2 – OTA REV105](../ota/ESP32_2_REV105_CORE3_BENCH.bin)
