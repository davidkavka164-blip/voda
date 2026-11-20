# Nouzové ruční ovládání ventilů — fyzický reléový výstup REV17

## Rozsah REV17

REV17 navazuje na logický a webový nouzový ruční režim REV16. Nově propojuje ruční příkaz se dvěma reléovými výstupy ESP32 #1. Nezapojuje fyzické výstupy do automatického transferu; ten zůstává pouze simulací. Relé čerpadel zůstávají neovládána.

## Přiřazení výstupů

| Funkce | ESP32 #1 GPIO | Logický aktivní stav |
|---|---:|---|
| Relé cesty studna → dolní nádrž | GPIO16 | HIGH = relé sepnuté |
| Relé cesty horní → dolní nádrž | GPIO17 | HIGH = relé sepnuté |

Přiřazení navazuje na provizorní pin map rev. C, která již byla v REV16 uvedena. ESP32 #2 ventily neovládá.

Uživatelem potvrzená vlastnost ventilů: **bez napájení jsou zavřené**. Firmware proto při CLOSE_ALL vypíná obě relé; při otevření směru sepne jen odpovídající relé. Pro dosažení tohoto chování musí být cívka ventilu napájena přes kontakt COM–NO příslušného relé. Zapojení COM/NO/NC je nutné před připojením vody ověřit měřením na konkrétním modulu.

## Bezpečné chování ve firmwaru

- GPIO16/17 se inicializují na LOW před startup delay, síťovou inicializací a NVS operacemi.
- Reléová deska je napájena 5 V (upřesnění potvrzené uživatelem). Řídicí vstup je aktivní HIGH; LOW znamená relé bez buzení. 5 V je napájení desky, nikoli automaticky napětí cívek ventilů.
- Driver při inicializaci nastaví obě relé OFF.
- Otevření jedné cesty nejprve vypne obě relé a až poté sepne vybrané relé (break-before-make).
- Opakovaný stejný povel se znovu nezapisuje, aby relé necvakala v každém průchodu loop.
- STOP, ztráta bezpečnostní podmínky a ukončení ručního režimu nastaví oba výstupy OFF.
- Ruční režim je pouze RAM; po restartu je neaktivní a oba relé výstupy jsou nastaveny OFF.
- Pokud výstupní driver není připraven, otevření je blokováno.
- Bez koncových spínačů/zpětné vazby se web zobrazuje jako povel relé, nikoli potvrzená fyzická poloha ventilu.

## Co zůstává vypnuté / nedokončené

- Automatický transfer je stále SIMULACE ONLY a jeho TransferLogic neovládá relé.
- Relé čerpadel nejsou inicializována ani ovládána.
- ESP32 #2 nepřebírá ovládání ventilů.
- Fyzická poloha ventilů není měřena.
- Fyzické chování při výpadku napájení, resetu a přerušení vodiče nebylo laboratorně ověřeno.

## Povinné hardwarové kontroly před připojením ventilů

1. Ověřit, že vstupy konkrétního 5V napájeného relé modulu spolehlivě reagují na 3,3V HIGH z ESP32; 5 V napájí desku, 3,3 V je řídicí signál a jeho kompatibilita zatím není fyzicky ověřena.
2. Doplnit externí pull-down rezistor (doporučeně 10 kΩ) z každého vstupu relé IN na GND, aby aktivně-HIGH relé zůstalo vypnuté i během resetu/bootu, kdy je GPIO ve vysoké impedanci.
3. Ověřit, že cívka ventilu je zapojena přes COM–NO, takže nebuzené relé odpojí napájení cívky. Nepoužívat NC kontakt pro napájení cívky.
4. Ověřit napětí a proud cívek, zdroj, jištění, ochranné prvky proti indukční špičce a společné/oddělené země podle skutečného relé modulu.
5. Nejprve testovat relé bez připojených ventilů a bez vody; ověřit reset, odpojení napájení ESP32, odpojení 5V relé modulu, přerušení řídicího vodiče a STOP.
6. Teprve poté připojit ventily a provést test zavření bez tlaku/průtoku. Bez zpětné vazby firmware nesmí tvrdit, že ventil skutečně dosáhl zavřené polohy.

Software sám nemůže garantovat stav GPIO během hardwarového resetu. Externí pull-down a ověření reléové desky jsou proto nutnou podmínkou fyzického provozu.

## Zachované principy

Beze změny zůstávají všechny hladinové hodnoty, kritické limity ochrany čerpadel, householdReserveCm, transferValveOpenCm, ventilová hystereze, logika ESP32 #2, ochrana při ztrátě peer komunikace a pravidlo, že pumpProtectionReady je diagnostický údaj. Ruční režim nadále zachovává dostupné bezpečnostní brány z REV16.

## Testy

Nový hostitelský test ověřuje inicializaci OFF, odmítnutí neplatné konfigurace, mapování obou cest, aktivní HIGH/LOW polaritu, break-before-make, idempotenci opakovaného povelu a návrat do CLOSE_ALL. Testy neprokazují elektrické chování konkrétního relé modulu ani ventilu.
