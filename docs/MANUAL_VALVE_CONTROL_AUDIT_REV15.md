# HOME WATER CONTROLLER — audit ručního ovládání ventilů

**Auditní revize:** 1.00 / REV15  
**Výchozí balíček:** DEV KIT 1.00 AUDIT DRAFT REV14  
**Autoritativní požadavky:** `HOME_WATER_CONTROLLER_MASTER_1.3`  
**Rozsah:** audit a návrh; bez změn firmwaru a bez aktivace fyzických výstupů.

## 1. Závěr

Ruční ovládání ventilů požadované Masterem 1.3 není v REV14 implementováno. Existující nouzové ovládání `/pump-emergency` je určeno pro logiku ochrany čerpadla, nikoli pro ventily. Transferová obrazovka ESP32 #1 je výslovně pouze simulace. V balíčku není dokončená fyzická výstupní vrstva ventilů.

Proto není bezpečné doplnit pouze webová tlačítka, která by přímo přepínala GPIO. Nejprve je nutné dokončit samostatnou logiku ručního režimu, určit zachované ochrany a ověřit skutečné zapojení ventilů, relé a případné zpětné vazby.

## 2. Požadavky přímo stanovené Masterem 1.3

| Zdroj v Masteru | Požadavek |
|---|---|
| § 39 Ventily | Stavy CLOSED, OPENING, OPEN, CLOSING, ERROR, BLOCKED. Bez fyzické zpětné vazby se nesmí tvrdit dosažení polohy; lze zobrazit jen stav vydaného povelu. Při bezpečnostním ukončení se vydá povel k uzavření. |
| § 40 Ruční ovládání ventilů | Web umožní ruční ovládání pro servis, test, údržbu, diagnostiku a nouzový provoz. Vyžaduje vědomý příkaz. Při vadném hladinovém čidle se zobrazí upozornění na nutnost fyzické kontroly hladiny. Nouzové ovládání neobchází ochrany, které lze zachovat. |
| § 52 Web – nouzový provoz | Při poruše čidla se zobrazí samostatné označení „NOUZOVÉ RUČNÍ OVLÁDÁNÍ VENTILŮ“. Aktivace je ruční, vyžaduje potvrzení, výrazné označení a fyzickou kontrolu hladiny. Zachovají se ostatní dostupné ochrany. Běžný provoz se neobnoví automaticky před ověřením čidla. |
| § 53 Ruční STOP | Transfer lze kdykoliv zastavit; systém vydá povel k uzavření a uloží ukončení uživatelem. Bez zpětné vazby se netvrdí neověřené fyzické uzavření. |
| § 56 Výpadek komunikace | Bez spolehlivých údajů horní nádrže se horní transfer automaticky nespustí ani nepokračuje. ESP32 #2 zachová místní bezpečnostní funkce. Obnovení komunikace samo nespustí nový transfer. Text výslovně popisuje automatický transfer; nevymezuje úplně chování ručního nouzového příkazu při výpadku peer komunikace. |
| § 60–61 Transfer | Bezpečnostní ukončení má přednost před požadavkem uživatele; při ukončení se vydá povel k uzavření a výsledek se označí jako neúplný, je-li to relevantní. |
| § 6 Architektura | Všechny elektrické ventily u dolní nádrže ovládá ESP32 #1. ESP32 #2 obsluhuje horní nádrž a její místní ochranu čerpadla. |

Další základní pravidla: povolené směry jsou studna → dolní nádrž a horní nádrž → dolní nádrž; automatický transfer dolní → horní není požadován. Čerpadla běžně řídí vlastní tlaková automatika.

## 3. Co je skutečně v REV14

### 3.1 ESP32 #1

- Hlavní webová stránka zobrazuje stav studny, dolní a horní nádrže, ochrany a simulace transferu.
- Existují POST endpointy `/transfer-start`, `/transfer-stop`, `/transfer-recovery-ack` a `/pump-emergency`.
- `/transfer-start` a `/transfer-stop` ovládají simulační automat transferu. README `firmware/ESP32_1/README.md` výslovně uvádí, že transferová obrazovka je pouze SIMULAČNÍ a neovládá fyzické výstupy.
- `/pump-emergency` přijímá `mode=enable|disable`; při aktivaci vyžaduje potvrzení fyzické kontroly hladiny studny. Handler pracuje s `WellPumpProtection`; není to příkaz ventilu.
- Simulační `TransferLogic` vydává logické povely `VALVES_CLOSE_ALL`, `VALVE_OPEN_WELL_TO_LOWER` a `VALVE_OPEN_UPPER_TO_LOWER`. Tyto hodnoty nejsou fyzickým výstupem.
- Není zde samostatný stavový automat ručního ovládání ventilů, ruční endpoint pro ventily ani dokončený ovladač fyzických ventilů.

### 3.2 ESP32 #2

- Webová stránka a `/pump-emergency` se týkají nouzového režimu ochrany čerpadla horní nádrže.
- ESP32 #2 neposkytuje webové ruční ovládání ventilů; podle Masteru navíc všechny ventily fyzicky obsluhuje ESP32 #1.

### 3.3 Hardware a výstupy

- `firmware/ESP32_1/src/HardwarePins.h` obsahuje pouze **provisional GPIO map rev. C**. Samotný soubor výslovně upozorňuje, že definice pinu neznamená aktivní výstup.
- `ESP32_1.ino` uvádí, že výstupy čerpadel a ventilů zůstávají vypnuté. V kódu nejsou nalezeny inicializace ani zápisy GPIO pro relé ventilů.
- Neexistuje ověřená zpětná vazba polohy ventilů. Proto nelze hlásit skutečné OPEN/CLOSED.
- Webové POST handlery používají `requireWebAuth()`. Budoucí ruční endpointy musí zachovat autorizaci a explicitní POST příkazy.

## 4. Rozlišení tří samostatných funkcí

1. **Automatický transfer:** požadovaný směr a objem, předběžné kontroly, průběžné hlídání hladin a bezpečnostní ukončení. REV14 má tuto funkci pouze simulačně.
2. **Nouzový režim čerpadla:** vědomé nouzové povolení logiky ochrany čerpadla při vadném čidle, s fyzickou kontrolou hladiny. Endpoint `/pump-emergency` patří výhradně sem.
3. **Ruční ovládání ventilů:** samostatná funkce pro servis/test/údržbu/diagnostiku a nouzový provoz. Vyžaduje vlastní potvrzení a stavové řízení. Nesmí se implementovat jako vedlejší větev `/pump-emergency` ani jako obejití `TransferLogic` přímým GPIO příkazem.

## 5. Bezpečný návrh — návrh, nikoli požadavek již schválený v Masteru

### 5.1 Oddělený stavový automat

Doporučený nezávislý automat `ManualValveControl` na ESP32 #1:

- `MANUAL_IDLE` — ruční režim neaktivní; žádný ruční povel k otevření.
- `MANUAL_REQUESTED` — uživatel vybral požadovanou servisní operaci/směr.
- `MANUAL_CONFIRMATION_REQUIRED` — zobrazení varování a výslovné potvrzení; při vadném čidle také potvrzení fyzické kontroly relevantních hladin.
- `MANUAL_PRECHECK` — vyhodnocení všech ochran, které jsou v dané situaci stále spolehlivě dostupné.
- `MANUAL_ACTIVE` — ruční povel je aktivní; stav se průběžně přehodnocuje.
- `MANUAL_STOPPING` — vydán povel CLOSE; bez snímače polohy nejde o potvrzený stav CLOSED.
- `MANUAL_BLOCKED` — operace odmítnuta kvůli bezpečnostní podmínce.
- `MANUAL_ERROR` — chyba výstupní vrstvy, nejednoznačný stav nebo ztráta ovladatelnosti.

Toto je návrh pojmenování a členění; neznamená to, že již existují tyto stavy nebo že je schválen jejich přesný přechodový diagram.

### 5.2 Oddělení od automatického transferu

- Ruční režim nesmí převzít nebo tiše obnovit rozpracovaný automatický transfer.
- Před ruční aktivací musí být automatický transfer v neaktivním/ukončeném stavu a musí být vyloučen konflikt dvou cest.
- Běžný transfer nesmí pokračovat souběžně s ručním povelem.
- Ukončení ručního režimu nesmí samo spustit automatický transfer. Nový automatický transfer vyžaduje nový požadavek uživatele.
- Ruční povel musí mít jednoznačný cíl. Nesmí být možné současně vydat konfliktní povely více cestám.

### 5.3 Zachování ochran

- Bezpečnostní podmínky, jejichž měření a vyhodnocení zůstávají spolehlivé, musí zůstat aktivní i v ručním režimu.
- Vadné čidlo se nesmí označit jako platné. Web musí zobrazit konkrétní vadné/neplatné měření a varování o fyzické kontrole hladiny.
- Ruční potvrzení uživatele není digitální měření a nesmí se ukládat jako validní hodnota čidla.
- Kritická blokace čerpadla, rezerva domácnosti a práh otevření horního ventilu zůstávají oddělené pojmy. Tento audit nenavrhuje změnu jejich hodnot ani významu.
- `pumpProtectionReady` zůstává diagnostickým údajem; tento audit z něj nedělá novou podmínku ručního ventilu.
- Pokud bezpečnostní ochranu nelze kvůli vadnému snímači zachovat automaticky, musí být její omezení viditelně oznámeno. Zda fyzická kontrola uživatele v konkrétním scénáři dovolí ruční otevření, není ve všech kombinacích poruch výslovně stanoveno; viz otevřené body.

### 5.4 Webové rozhraní

Navrhovaná samostatná sekce „Ruční ovládání ventilů“ na hlavním webu ESP32 #1:

- trvalé a výrazné označení `RUČNÍ REŽIM AKTIVNÍ` nebo `NOUZOVÉ RUČNÍ OVLÁDÁNÍ VENTILŮ`;
- zobrazení důvodu aktivace, platnosti/čerstvosti každého relevantního čidla a stavu komunikace;
- zobrazení vybraného směru/ventilu a povelu, nikoli neověřené fyzické polohy;
- samostatné potvrzení před aktivací; při vadném čidle výslovné potvrzení fyzické kontroly hladiny;
- samostatný STOP, který okamžitě vydá CLOSE povel;
- jasné oznámení, které automatické funkce jsou zablokovány během ručního režimu;
- po skončení režimu žádné automatické obnovení transferu.

Názvy budoucích endpointů nejsou stanoveny a v této revizi se žádné nepřidávají.

## 6. Bezpečnostní scénáře k pokrytí před implementací

| Scénář | Požadované bezpečné chování / auditní závěr |
|---|---|
| Uživatel aktivuje ruční režim při platných čidlech | Vědomý příkaz a potvrzení; stále zachovat dostupné limity a kontrolu konfliktu. Přesné možnosti servisního testu musí odpovídat skutečnému zapojení ventilů. |
| Porucha čidla zdroje | Výslovně označit nouzový ruční režim a vyžadovat fyzickou kontrolu hladiny. Neoznačit vadné měření jako validní. Zachovat ostatní dostupné ochrany. |
| Porucha čidla dolní nádrže | Nelze automaticky ověřit volnou kapacitu cíle. Master požaduje fyzickou kontrolu při vadném čidle, ale přesná politika, zda a za jakých podmínek smí ruční ventil otevřít při neznámé kapacitě cíle, není jednoznačně rozpracována. |
| Výpadek peer komunikace při ručním horním transferu | Automatický horní transfer musí být blokován/ukončen dle § 56. Master neříká jednoznačně, zda fyzická kontrola uživatele může nahradit chybějící peer údaje pro ruční nouzový příkaz. Nesmí se to domyslet v implementaci. |
| Restart ESP32 #1 během ručního režimu | Ruční režim se po restartu nesmí automaticky obnovit. Po startu nesmí software tvrdit CLOSED bez polohové zpětné vazby. Fyzický bezpečný stav po resetu musí být určen hardwarem a ověřen. |
| Restart ESP32 #2 | Místní ochrana čerpadla #2 zůstává lokální. Nový boot peeru nesmí obnovit starý ruční/automatický požadavek bez nové akce uživatele. |
| Ztráta napájení / reset výstupního relé | Bez ověřeného typu ventilu a zapojení nelze určit, zda se ventil fyzicky zavře, zůstane v poslední poloze nebo se jinak zachová. Nutné potvrdit hardware. |
| Ruční STOP | Vydat CLOSE povel a zaznamenat uživatelské ukončení. Bez feedbacku zobrazit „povel k uzavření vydán“, nikoli „ventil uzavřen“. |
| Konflikt s automatickým transferem | Ruční a automatický režim nesmějí současně ovládat ventil. Předání řízení musí být explicitní a bezpečně ukončit předchozí režim. |
| Vadná/nezpětně hlášená poloha ventilu | Stav ERROR nebo neověřený povel podle skutečných diagnostických možností. Bez snímače polohy nelze odlišit mechanické zaseknutí od úspěšného pohybu. |
| Obnovení čidla | Běžný provoz se neobnoví automaticky před ověřením čidla. Nouzový režim musí uživatel ukončit vědomě; poté ověřit platnost měření a standardní ochrany. |

## 7. Co musí být doplněno před implementací

1. Ověřená topologie: kolik ventilů je skutečně instalováno, co každý ventil odděluje a které kombinace otevření jsou fyzicky přípustné.
2. Typ a elektrické parametry každého ventilu: napájení, proud, typ ovládání, klidový stav, zda je monostabilní/bistabilní a co se stane při ztrátě napájení.
3. Ověřený pinout a reléový modul včetně polarity, galvanického oddělení, ochrany a bezpečného stavu při resetu. Prozatímní GPIO map rev. C není potvrzené zapojení.
4. Zda ventily poskytují koncové spínače nebo jinou zpětnou vazbu polohy.
5. Přesná politika pro ruční horní transfer při ztrátě peer komunikace a pro ruční otevření při neplatném čidle dolní nádrže.
6. Jaké servisní operace vyžadují ovládání jednotlivého ventilu a jaké pouze předdefinované cesty. Master tyto detaily nezakládá.

## 8. Testovací plán před povolením fyzických výstupů

- Bez potvrzení se ruční režim neaktivuje.
- Aktivace s potvrzením fyzické kontroly je zaznamenána a výrazně označena.
- Automatický transfer a ruční ovládání se vzájemně vylučují.
- Ruční STOP vždy vydá CLOSE povel a vytvoří auditní záznam.
- Neplatné/zastaralé údaje, kritické alarmy, konfliktní stav a chybějící konfigurace vedou k definovanému blokování nebo explicitnímu nouzovému postupu; žádná ochrana se tiše nevypne.
- Restart obou jednotek, ztráta/obnovení peer komunikace, výpadek Wi-Fi/internetu a restart při každém ručním stavu.
- Chování při resetu, výpadku napájení, přerušení vodiče a zaseknutí ventilu ověřit na bezpečném zkušebním zapojení bez připojeného zdroje vody.
- Zpětná vazba: nikdy nehlásit fyzické OPEN/CLOSED, pokud není tato poloha skutečně měřena.
- Teprve po úspěšném softwarovém a laboratorním ověření provést samostatnou hardwarovou revizi a schválení aktivace výstupů.

## 9. Rozsah této revize

- Zdrojový firmware nebyl změněn.
- Žádný GPIO, ventil ani čerpadlo nebyly aktivovány.
- Nebyly změněny hladinové prahy, rezerva domácnosti, kritické limity, hystereze ani automatická logika transferu.
- Dokument popisuje požadavky Masteru, ověřený stav REV14 a návrhy. Návrhy nejsou vydávány za již schválené vlastnosti.
- Předchozí testy REV14 zůstávají výsledkem testování zdrojů REV14; tento dokument nepřidává ani nepředstírá testy fyzického ovládání ventilů.
