# HOME WATER CONTROLLER — pracovní návrh GPIO (revize C)

Stav: **pracovní návrh pro klasické ESP32 DevKit / ESP32-WROOM-32**. Piny jsou přiřazeny v `firmware/common/HardwarePins.h` a zkopírovány do `src` obou Arduino sketchů. Uživatel potvrdil typ 5V reléového modulu pro ventily; mapa stále není schválená pro provozní řízení, protože není ověřeno chování ventilů a fyzické zapojení kontaktů.

## Potvrzený hardware

### Bezpečnostní relé čerpadel
Uživatel potvrdil modul Drátek.cz **Modul relé 24V 30A HIGH–LOW, 1 kanál na DIN lištu**, produktové číslo **1476828250**. Modul má napájení 24 V DC, optočlen, kontaktové parametry uváděné pro 250 VAC / 30 VDC a přepínání aktivní úrovně HIGH/LOW. Uživatel potvrdil, že stejné zapojení funguje v samostatném projektu studny; pro všechny tři pumpové výstupy se proto přebírá **24 V DC, jumper HIGH, aktivní HIGH z GPIO ESP32**. HIGH povoluje napájení motoru, LOW jej přerušuje. Není požadována nová změna zapojení ani doplnění dalších součástek.

Odkaz výrobce/prodejce: https://dratek.cz/arduino-platforma/74588-modul-rele-24v-30a-high-low-1-kanal-na-listu.html


### Relé elektrických ventilů
Uživatel potvrdil modul Drátek.cz **„Modul relé 5V – 1 kanál s úrovňovým spínačem“**, produkt **1517435132**. Výrobce/prodejce uvádí optické oddělení, napájení 5 V, spínací vstup přibližně 5 mA, kontakty 250 VAC / 10 A a volbu LOW/HIGH pro aktivní logickou úroveň. Pro návrh počítáme s jumperem v poloze **HIGH**, tedy aktivací logickou jedničkou. Zdroj: https://dratek.cz/arduino-platforma/2947-modul-rele-5v-1-kanal-s-urovnovym-spinacem.html.

Před provozem je nutné ověřit, že vstup spolehlivě reaguje na 3,3 V z ESP32 a že výstupní relé zůstává neaktivní při resetu/bootu. Samotné optické oddělení automaticky neznamená galvanické oddělení při libovolném způsobu zapojení napájení.

### Snímače hladiny
Pro čtyři dříve objednaná čidla QDY30A je znám rozsah 0–10 m a signál 4–20 mA / napájení 24 V DC. Měřicí obvod 150 Ω je pouze dosavadní návrh: 4 mA → 0,6 V, 20 mA → 3,0 V. Před připojením ESP32 ověřit skutečné napětí, polaritu a společnou referenci. Proudový výstup se nesmí připojit přímo na GPIO.

## ESP32 č. 1 — hlavní jednotka

| Funkce | GPIO | Poznámka |
|---|---:|---|
| W5500 SCK | 18 | SPI |
| W5500 MISO | 19 | SPI |
| W5500 MOSI | 23 | SPI |
| W5500 CS | 27 | Samostatný výběr čipu |
| W5500 RST | 26 | Použít jen pokud je reset modulu vyveden |
| Hladinové čidlo studny / ADC | 34 | ADC1, pouze vstup |
| Hladinové čidlo dolní nádrže / ADC | 35 | ADC1, pouze vstup |
| Relé elektrického ventilu 1 | 16 | Drátek 5V s úrovňovým spínačem; jumper HIGH (ověřit 3,3V vstup) |
| Relé elektrického ventilu 2 | 17 | Drátek 5V s úrovňovým spínačem; jumper HIGH (ověřit 3,3V vstup) |
| Bezpečnostní relé čerpadla studny | 25 | Drátek 24V/30A; HIGH aktivní; samostatná ochrana studny |
| Bezpečnostní relé čerpadla dolní nádrže | 32 | Drátek 24V/30A; HIGH aktivní; samostatná ochrana dolní nádrže |

## ESP32 č. 2 — horní nádrž

| Funkce | GPIO | Poznámka |
|---|---:|---|
| W5500 SCK | 18 | SPI |
| W5500 MISO | 19 | SPI |
| W5500 MOSI | 23 | SPI |
| W5500 CS | 27 | Samostatný výběr čipu |
| W5500 RST | 26 | Použít jen pokud je reset modulu vyveden |
| Hladinové čidlo horní nádrže / ADC | 34 | ADC1, pouze vstup |
| Teplotní čidlo | 4 | Předpokládá DS18B20; potvrdit typ |
| Bezpečnostní relé čerpadla horní nádrže | 25 | HIGH aktivní; místní ochrana na ESP32 #2 |

## Bezpečnostní pravidla

- Potvrzené přiřazení: ESP32 #1 GPIO25 = čerpadlo studny; ESP32 #1 GPIO32 = čerpadlo dolní nádrže; ESP32 #2 GPIO25 = čerpadlo horní nádrže. Všechna tři čerpadla mají samostatnou ochranu.
- Uživatel potvrdil, že pump safety relé přerušují napájení motorů a výkonové dimenzování je dostačující. Funkční zapojení stejného modulu bylo uživatelem potvrzeno v samostatném projektu studny. REV28 přebírá napájení 24 V DC, jumper HIGH, aktivní HIGH z GPIO a povoluje všechny tři fyzické výstupy; viz `RELEASE_NOTES_1.00_AUDIT_REV28.md`.
- Ochrana čerpadla dolní nádrže musí mít vlastní kritickou hladinu a hysterezi. Nelze automaticky převzít informativní údaj 11 cm, který nesmí být použit jako řídicí práh.

1. GPIO nesmí přímo napájet cívku 5V/24V relé. Použít odpovídající reléový modul/budič.
2. U pumpových relé se použije již funkční zapojení ze samostatné studny: napájení **24 V DC**, jumper **HIGH**, GPIO HIGH povoluje motor a GPIO LOW motor odpojuje.
3. Firmware nastavuje pump safety GPIO do LOW na začátku `setup()`. Další součástky ani změna zapojení se nepřidávají; přebírá se ověřené zapojení samostatné studny.
4. Zatím neaktivovat výstupy ventilů v provozním kódu: neznáme přesný typ 5V reléových modulů ani chování ventilů při ztrátě napájení.
5. U pumpových kontaktů se přebírá již funkční zapojení, kterým relé přerušuje napájení motoru. Stejné zapojení se použije u všech tří čerpadel.
6. W5500 moduly musí používat kompatibilní napájení a 3,3V SPI logiku. Obě jednotky se propojí přes společnou LAN/switch; dva W5500 samy o sobě netvoří přímou komunikační linku.
7. GPIO34/35 jsou vstupní-only a nemají interní pull-up/pull-down; to je pro analogové měření vhodné.
8. Před fyzickým testem potvrdit, že obě desky jsou klasické ESP32 DevKit / ESP32-WROOM-32 a že skutečné moduly W5500 odpovídají navrženému SPI rozhraní.

## Zbývající otevřené položky

- Ověřit 3,3V ovládání 5V reléových modulů pro ventily a neaktivní stav při bootu.
- Typ ventilů (monostabilní/bistabilní, NO/NC) a požadovaný stav při ztrátě napájení.
- Potvrdit typ teplotního čidla a model W5500.
- Potvrdit přiřazení jednotlivých snímačů a zda čtvrté čidlo zůstane rezervní.

**Tato mapa je návrhem pinů, nikoli potvrzením kompletního bezpečného elektrického zapojení.**


## Aktualizace REV45 – síťová topologie

Starší návrh pinů W5500 pro obě desky je pro ESP32 #1 síťově zastaralý. Potvrzená topologie je ESP32 #1 přes vestavěnou Wi-Fi STA a ESP32 #2 přes W5500 Ethernet. W5500 SPI piny na ESP32 #1 proto nejsou v aktuální peer síťové cestě použity; pinový návrh W5500 zůstává relevantní pouze pro ESP32 #2.


## Aktualizace REV85 — W5500 ESP32 #1

Starší síťová poznámka REV45 výše je historická. Od REV85 firmware ESP32 #1 používá W5500 na vyhrazených pinech GPIO18 (SCK), GPIO19 (MISO), GPIO23 (MOSI), GPIO27 (CS) a GPIO26 (RESET) pro DHCP Ethernet. Wi-Fi STA a konfigurační AP zůstávají zachovány. Produkční HTTP ovládání přes W5500 zatím není implementováno.
