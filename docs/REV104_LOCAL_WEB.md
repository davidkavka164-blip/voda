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

## REV106 – kalibrace objemu horní nádrže

ESP32 #2 opět poskytuje objemovou kalibraci nepravidelné horní nádrže.
Pod nákresem ukazuje celkový objem vody při aktuální hladině jako
kalibrační bod nebo odhad mezi sousedními body. Mimo kalibrovaný rozsah
a bez platné čerstvé hladiny je objem nedostupný. Samotný nákres stále
představuje výšku hladiny.

1. V části **Objemová kalibrace horní nádrže** ulož první známou dvojici:
   aktuální hladinu a celkový objem ověřený nezávisle. Pokud je objem
   kladný, stejný vzorek se uloží také jako výchozí stav přepouštění.
2. Po přepouštění zadej na ESP32 #2 odhad skutečného přírůstku dolní nádrže
   z historie ESP32 #1, tedy výsledek podle změny hladiny, nikoli cílový
   počet litrů. Mezi záznamy v obou nádržích nesmí proběhnout jiný přítok
   ani odběr.
3. ESP32 #2 od výchozího objemu odečte přepuštěné litry a uloží bod
   pro novou hladinu. Před dalším přepouštěním ulož novou referenci.
   Body se ukládají do stávajícího úložiště a ověřují zpětným načtením.

Záznamy z dřívější kalibrace se znovu použijí, pokud projdou ověřením.
Položky rezervy domácnosti zůstávají mimo web a ochrana čerpadla pracuje
s hladinou. Jednotky si data neposílají.

- [ESP32 #2 – OTA REV106](../ota/ESP32_2_REV106_CORE3_BENCH.bin)
