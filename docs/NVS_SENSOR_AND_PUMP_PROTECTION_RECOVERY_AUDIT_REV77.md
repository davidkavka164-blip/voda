# NVS audit: electrical sensor calibration and pump protection — REV77

**Datum:** 15. 11. 2026  
**Základ:** DEV KIT 1.00 REV76  
**Rozsah:** elektrická kalibrace snímačů, bezpečnostní limity a trvalé kritické blokace čerpadel. Nejde o fyzický test NVS.

## 1. Výsledek v kostce

- Elektrická kalibrace všech tří hladinových snímačů je podmínkou použití výšky pro bezpečnostní logiku. Chybějící, poškozený nebo nedostupný záznam znamená blokaci příslušné ochrany.
- Servisní obnova elektrické kalibrace existuje. Platný záznam nelze přes tuto cestu přepsat. Obnova vyžaduje ověřené měřítko, nezávisle fyzicky změřenou hladinu a potvrzení obsluhy. Po zápisu se zneplatní starý vzorek a čeká se na nový vzorek z ověřené konfigurace.
- Kritické blokace a bezpečnostní stav čerpadel jsou ukládány odděleně od elektrické kalibrace. Dvouslotové úložiště odmítá načíst záznam, pokud je některý přítomný slot poškozený nebo je dvojice generací nejednoznačná. Neprovádí automatický návrat ke staršímu slotu.
- Limity studniční a dolní pumpy jsou chráněny proti automatickému přepsání poškozeného záznamu. Horní limity mají CRC a stejnou ochranu proti přepsání, ale zatím používají jediný blob, nikoli dvouslotový žurnál.
- **Žádný z poškozených bezpečnostních záznamů se nesmí automaticky smazat, vynulovat ani nahradit výchozími hodnotami.** Nouzový režim není servisní opravou NVS.

## 2. Matice uložených dat

| Data | Úložiště | Ochrana zápisu / čtení | Chování při chybě |
|---|---|---|---|
| Elektrická kalibrace studny, dolní a horní nádrže | `NvsSensorConfigStore`, klíče `well`, `lower`, `upper` v `hwc_cfg` | přesná délka, verze, rozsah, CRC, zpětné načtení; běžný zápis nepřepíše neplatný existující záznam | výška je pouze diagnostická; příslušná ochrana čerpadla/transferu zůstává blokována |
| Ochrana čerpadla studny | `NvsWellProtectionStore`, `hwc_protect`, `slot0/slot1` | dva sloty, generace, CRC, validace payloadu, read-back; kritická západka i `safetyStateValid` jsou součástí záznamu | chyba kteréhokoli slotu nebo nejednoznačné generace = fail-closed; bez automatické opravy |
| Ochrana čerpadla dolní nádrže | `NvsLowerTankPumpProtectionStore`, `hwc_lowpump`, `slot0/slot1` | dva sloty, generace, CRC, validace payloadu, read-back; zápis vyžaduje platnou konfiguraci a platný bezpečnostní stav | poškozený slot blokuje načtení i další zápis; čerpadlo zůstává blokováno |
| Bezpečnostní západka horního čerpadla | `NvsUpperTankProtectionStore`, `hwc_up_safety`, `slot0/slot1` | dva sloty, generace, CRC, validace a read-back; odděleno od nastavitelných limitů | chybějící/poškozený stav se nepovažuje za potvrzené odjištění; pump permit zůstává blokován |
| Limity horní nádrže | `NvsUpperTankLimitsStore`, `hwc_upperlimits`, klíč `state` | jeden blob s verzí a CRC, read-back; existující poškozený záznam nelze běžným formulářem přepsat | limity jsou neplatné a horní pumpa zůstává blokována; není zde servisní UI pro obnovu poškozeného blobu |
| Geometrie studny a objemová kalibrace studny | samostatné CRC blob stores | délka, validace, CRC, read-back, odmítnutí přepsání poškozeného záznamu | ztráta odhadu/kalibrace objemu; nesmí měnit povolení pumpy ani transferu |

## 3. Elektrická kalibrace snímače

### Běžný provoz

`SENSOR_CONFIG_VALID_STORED` je jediný stav, který dovoluje použít převod 4–20 mA → výška pro bezpečnostní logiku. Výchozí hodnoty jsou pouze diagnostické. Stav `MISSING_PROVISIONAL`, `INVALID_PRESENT` nebo `NVS_UNAVAILABLE` není bezpečnostně použitelný.

### Servisní obnova

- Obnova je dostupná přes autentizované webové rozhraní příslušné jednotky a vyžaduje potvrzení fyzického měření.
- Uživatel musí zadat měřítko, které už zná z ověřené kalibrace. Jediný referenční bod vypočte pouze offset; nesmí se z něj odhadovat neznámé měřítko.
- Firmware použije aktuální platný elektrický vzorek, odvodí offset, zkontroluje rozsahy, uloží celý záznam a načte jej zpět.
- `recover()` nesmí přepsat platný záznam. Při chybě/krátkém čtení existujícího záznamu, nedostupném NVS, chybě zápisu nebo read-back se obnova odmítne.
- Po úspěšném zápisu se zneplatní cache starého vzorku. Ochrana se smí obnovit až po novém měření převedeném novou ověřenou konfigurací a po dosažení příslušné obnovovací hladiny.

**Omezení:** senzorový záznam je stále jeden blob. Hostitelský model pokrývá simulované částečné/nejisté zápisy, ale nedokazuje chování konkrétní verze ESP32 NVS při fyzickém brownoutu.

## 4. Bezpečnostní limity a kritická západka

### Dvouslotové ochranné záznamy

Studna, dolní čerpadlo a horní bezpečnostní západka používají dva nezávislé klíče. Nový zápis cílí na druhý slot, ověřuje úplné načtení a kontrolní součet. Generace musí být po sobě jdoucí; přetečení na nulu se odmítá.

Záměrně přísná politika znamená, že i když je druhý slot poškozen a první stále obsahuje platný starší záznam, celý store vrátí chybu. Firmware neodhaduje, zda je starší latch nebo konfigurace bezpečná. To může snížit dostupnost, ale brání tichému návratu ke starší bezpečnostní konfiguraci nebo vymazání kritické západky.

### Horní nastavitelné limity

`NvsUpperTankLimitsStore` používá zatím jediný klíč `state`. CRC, délka a read-back chrání před přijetím poškozených dat a běžný webový zápis odmítne přepsat existující neplatný blob. Po přerušení zápisu může být jediný blob nečitelný; následkem je blokace horního čerpadla. Automatická servisní obnova ani reset z webu nejsou implementovány.

**Doporučený další technický krok:** migrovat horní limity na dvouslotový žurnál obdobný ochranným store, s bezpečným jednorázovým převodem starého `state` a s pravidlem, že při nejednoznačné migraci se čerpadlo nepovolí. Migrace musí zachovat všechny hodnoty a jejich pořadí, nesmí měnit kritickou západku a musí mít vlastní testy přerušení zápisu.

## 5. Servisní postup při poškozeném bezpečnostním záznamu

1. Nechat příslušné čerpadlo blokované a fyzicky izolovat jeho motor podle elektrického postupu instalace. Nespoléhat pouze na webový stav.
2. Ověřit hladinu nezávislou fyzickou metodou a zkontrolovat napájení, kabeláž a snímač.
3. Zachovat diagnostický výpis a kopii původních NVS klíčů. Neprovádět plošné `erase NVS`, nemažte jednotlivé sloty ručně a nezkoušejte zapsat odhadnuté bajty.
4. U elektrické kalibrace použít pouze popsanou servisní obnovu s nezávislou referencí a známým měřítkem.
5. U poškozených ochranných limitů nebo latch záznamů není v této revizi podporována webová oprava. Vyžaduje samostatně přezkoumaný servisní nástroj nebo nové uvedení ochrany do provozu; kritická západka se nesmí automaticky nulovat.
6. Před návratem do provozu ověřit studený start, validitu NVS, nový čerstvý vzorek, kritickou blokaci, obnovovací hysterezi a skutečnou funkci bezpečnostního relé.

Nouzové povolení čerpadla je samostatný, vědomý provozní bypass softwarových hladinových ochran. Není určen k opravě ani inicializaci NVS a neobchází elektrické/mechanické ochrany motoru.

## 6. Ověření a otevřené limity

- REV76 baseline: nativní C++ 78/78 PASS; Python 17/17 PASS; ASan/UBSan 76/76 PASS.
- V REV77 byly opakovány nativní C++ a Python testy: 78/78 a 17/17 PASS. ASan/UBSan běh v tomto prostředí překročil časový limit; výsledek se proto neprohlašuje za PASS.
- Testy jsou hostitelské modely a integrační kontroly. Nejsou náhradou za Arduino cílový překlad ani fyzické testy.
- Arduino IDE 1.8.19 / skutečná verze ESP32 core, fyzický power-cut test NVS, skutečné relé a měření čidel zůstávají neověřené.

**Závěr:** současné chování při chybě je konzervativní a fail-closed. Nejvýznamnější další zlepšení je dvouslotová ochrana `NvsUpperTankLimitsStore`; do její implementace se nesmí přidat automatické mazání nebo přepis poškozených limitů.
