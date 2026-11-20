# NVS storage audit — REV79

Datum: 17. 11. 2026 | Základ: DEV KIT 1.00 REV78

## Rozsah

Audit zahrnuje všechna produkční NVS úložiště použitá ve firmware ESP32 #1 a ESP32 #2, jejich validační a read-back postupy a reakci startovací logiky při chybě. Kontrola je zdrojová a hostitelská; neprokazuje chování konkrétního ESP-IDF/Arduino-ESP32 core při fyzickém výpadku napájení.

## Výsledek podle úložiště

| Úložiště | Ukládaná data | Strategie | Chování při chybě | Dopad |
|---|---|---|---|---|
| `NvsSensorConfigStore` (`hwc_cfg`) | Elektrická kalibrace studny, dolní a horní nádrže | Jednotlivé CRC záznamy, kontrola délky, read-back; běžné `save()` nepřepisuje existující neplatný záznam; servisní `recover()` má užší pravidla | Chybějící/neplatná/NVS nedostupná konfigurace je pouze diagnostická; výška se nepoužije pro normální ochranu | Bezpečnostně kritické, fail-closed |
| `NvsWellProtectionStore` | Limity a kritická západka studničního čerpadla | Dva střídající se sloty, generace, CRC, ověření zápisu; legacy migrace | Poškozený slot nebo nejednoznačná generace blokuje načtení; bez automatického rollbacku | Bezpečnostně kritické, fail-closed |
| `NvsLowerTankPumpProtectionStore` | Limity a kritická západka dolního čerpadla | Dva střídající se sloty, generace, CRC, ověření zápisu | Poškozený slot/nejednoznačná generace blokuje načtení | Bezpečnostně kritické, fail-closed |
| `NvsUpperTankProtectionStore` | Kritická západka a validita bezpečnostního stavu horního čerpadla | Dva střídající se sloty, generace, CRC, legacy migrace | Chybějící či poškozený stav blokuje běžné i nouzové povolení do bezpečné inicializace; žádné automatické vynulování západky | Bezpečnostně kritické, fail-closed |
| `NvsUpperTankLimitsStore` | Nastavitelné limity horní nádrže | REV78: transakční žurnál `slot0/slot1 + head + txn`, CRC, read-back, obnova přerušené transakce | Neúplný kandidát vrátí předchozí potvrzený stav; poškozený aktivní stav/head blokuje ochranu | Bezpečnostně kritické, fail-closed |
| `NvsTransferJournalStore` | Stav přenosu a restartová blokace | Dva střídající se sloty, generace, CRC, přesné porovnání po zápisu | Poškozený slot nebo nejednoznačný stav blokuje nové transfery; po restartu se aktivní přenos nikdy automaticky neobnoví | Bezpečnostně kritické, fail-closed |
| `NvsLowerTankInflowWatchdogStore` | Dva nastavitelné časy watchdogu horního → dolní transferu | Jeden verzovaný CRC blob, read-back a přesné porovnání obou hodnot | Chybějící/neplatná konfigurace nebo chyba NVS blokuje horní → dolní automatický transfer; výchozí 30/30 s není považováno za ověřené provozní nastavení | Bezpečnostně důležité, fail-closed |
| `NvsWellGeometryStore` | Průměr a využitelná hloubka studny | Jeden CRC blob, kontrola délky/rozsahu a read-back | Při chybě se použije výslovně označená výchozí geometrie pouze pro odhady a rozsahové kontroly; studniční ochrana stále vyžaduje samostatně platné uložené limity a elektrickou kalibraci | Konfigurační; chování odpovídá stávajícímu návrhu |
| `NvsWellCalibrationStore` | Měřená křivka výška → litry | Jeden CRC blob, validace, ochrana před přepsáním poškozeného záznamu a read-back | Při chybě se používá geometrický odhad; kalibrace objemu neuděluje ani neblokuje povolení čerpadla | Informativní výpočet objemu |
| `NvsUpperTankVolumeCalibrationStore` | Absolutní objemová křivka a rozpracovaná reference transferu | Jeden verzovaný CRC blob, úplné významové porovnání po read-back | Poškozený záznam blokuje kalibrační operace; odstranění poškozeného záznamu je samostatná výslovná servisní operace a vyžaduje novou absolutní kotvu | Informativní výpočet objemu, důležitá integrita reference |
| `NvsDailyInflowStore` | Denní a 30denní statistika přítoku | Jeden verzovaný CRC blob, kontrola délky/validity, ochrana před přepsáním poškozeného záznamu | Historie zůstane read-only/RAM-only; neovlivňuje ochranu čerpadel ani transferů | Nezabezpečnostní statistika |
| `NvsTransferHistoryStore` | Poslední výsledky transferů | Jeden verzovaný CRC blob, kontrola délky/validity, ochrana před přepsáním poškozeného záznamu | Historie je zablokována; samostatný transfer journal dál řídí restartovou blokaci | Nezabezpečnostní historie; nesmí být zaměňována za měření průtoku |

## Přímé zápisy mimo store

- Wi-Fi SSID/heslo ESP32 #1: návratové hodnoty `putString()` nejsou samostatně vyhodnoceny, ale obě hodnoty se po zápisu načtou zpět a přesně porovnají. Restart je povolen pouze při úspěšném read-back.
- Credential backend: zápisy kontrolují počet zapsaných bajtů a používají vlastní validační/ověřovací postupy; samostatné testy pokrývají přerušení a migraci.
- Ostatní nalezené `remove()`/`clear()` operace jsou součástí explicitní migrace, obnovy nebo mazání a nejsou automatickou cestou k vynulování pumpové západky.

## Zjištění a rozhodnutí REV79

1. V aktuálním produkčním startovacím toku nebyla nalezena cesta, která by při chybě NVS automaticky nastavila bezpečnostní limity, elektrickou kalibraci nebo kritickou západku na bezpečnou výchozí hodnotu a tím povolila normální činnost čerpadla.
2. Jednoslotová úložiště bezpečnostně důležitých nastavení (`SensorConfig`, watchdog) při poškození neposkytují návrat k předchozí verzi. Jejich současná reakce je ale fail-closed; NVS blob zápis má být atomický na cílové platformě, což zde nebylo možné fyzicky ověřit. Případný upgrade na žurnál se proto odkládá do ověření cílové verze Arduino-ESP32/ESP-IDF a skutečného chování NVS.
3. Objemové kalibrace, statistiky a historie zůstávají oddělené od bezpečnostních permitů. Zejména `WellCalibration` a `UpperTankVolumeCalibration` nesmí ovlivňovat ochranu čerpadel.
4. REV79 nemění firmware logiku, GPIO, relé, ventily, ochranné prahy ani nouzové režimy. Důvodem je, že audit nezjistil jednoznačnou chybu, která by opravňovala měnit bezpečnostní chování bez cílového testování.

## Neprovedené ověření

- Arduino-ESP32 cílový překlad.
- Testy NVS na skutečných deskách ESP32 s odpojením napájení v různých fázích zápisu.
- Fyzické ověření všech tří pumpových bezpečnostních relé, ventilů a senzorů.
- Zjištění přesné verze Arduino-ESP32 core a podkladové ESP-IDF.

Závěr: zdrojový audit REV79 nenašel důvod k zásahu do běhové bezpečnostní logiky. Dalším významným krokem je zjistit přesnou cílovou verzi core a provést na stole skutečné NVS/power-cut testy; teprve podle jejich výsledku rozhodnout, zda převést jednoslotová kritická úložiště na transakční žurnály.
