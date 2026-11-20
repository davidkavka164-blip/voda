# Audit bezpečnostních relé čerpadel – REV24

## Potvrzené rozdělení čerpadel

Uživatel upřesnil, že systém hlídá tři samostatná čerpadla:

| Jednotka | GPIO | Čerpadlo | Zdroj měření / ochrana |
|---|---:|---|---|
| ESP32 #1 | 25 | Čerpadlo ve studni | Hladinové čidlo studny; stávající `WellPumpProtection` |
| ESP32 #1 | 32 | Čerpadlo v dolní nádrži | Hladinové čidlo dolní nádrže; samostatná ochrana zatím chybí |
| ESP32 #2 | 25 | Čerpadlo horní nádrže | Hladinové čidlo horní nádrže; místní `UpperTankProtection` |

Čerpadla běžně spíná jejich vlastní tlaková automatika. ESP32 je bezpečnostně odstavuje při definované kritické hladině nebo jiné příslušné kritické závadě; nesmí převzít běžné tlakové řízení.

## Potvrzení výkonového relé

Uživatel potvrdil, že moduly Drátek 24 V DC / 30 A přerušují napájení motoru a jejich výkonové dimenzování je pro příslušná čerpadla dostačující. Tato informace nahrazuje předchozí otevřenou otázku REV23, zda relé přerušuje řídicí nebo výkonový obvod.

Před fyzickým uvedením do provozu stále musí být ověřeno konkrétní zapojení COM/NO/NC a výsledný stav motoru při:
- resetu nebo vypnutí ESP32,
- ztrátě 24V napájení reléového modulu,
- přerušení vodiče řídicího vstupu,
- startu firmware před dokončením validace ochrany.

Je nutné potvrdit, že bezpečnostní zákaz skutečně odpojí motor a že neaktivní/poruchový stav odpovídá požadovanému bezpečnému stavu. Vstupy modulů jsou nastaveny na HIGH logiku; kompatibilita s 3,3V GPIO a hardwarové předpětí při bootu musí být ověřeny na konkrétním zapojení.

## Důležitá chybějící část – čerpadlo dolní nádrže

Ve firmware je již implementována logika ochrany studničního čerpadla a místní logika ochrany horního čerpadla. Samostatná `LowerTankPumpProtection` pro čerpadlo dolní nádrže byla doplněna v REV25.

Ochrana dolního čerpadla je nezávislá na studniční ochraně a používá měření dolní nádrže. Má vlastní:
- kritickou hladinu pro odstavení čerpadla,
- obnovovací hladinu / hysterezi,
- stav kritického blokování (latch),
- validaci čidla a konfigurace,
- chování po restartu a při výpadku čidla,
- výslovný nouzový režim čerpadla podle pravidel hlavního dokumentu.

**Nesmí se převzít pracovní minimum dolní nádrže 11 cm jako kritická hladina čerpadla.** Pracovní minimum nádrže a bezpečnostní hranice čerpadla jsou rozdílné veličiny. Konkrétní minimální hladinu a hysterezi si nastaví obsluha ve webovém rozhraní. Bez platného uloženého nastavení zůstává logické povolení blokované.

## Nouzový režim ventilů a ochrany čerpadel

Nouzové ruční ovládání ventilů zůstává nezávislé na běžných transferových hladinových limitech. Nesmí ale vypínat ani obcházet ochranu čerpadel, která má platná data.

- Studniční trasa respektuje dostupnou ochranu studničního čerpadla.
- Horní čerpadlo je chráněno místně na ESP32 #2, nezávisle na komunikaci s ESP32 #1.
- Ochrana dolního čerpadla musí běžet místně na ESP32 #1, nezávisle na ochraně studny a na řízení ventilů.
- Pokud příslušné hladinové čidlo selže, nelze jeho hladinovou ochranu vydávat za funkční. Nouzový provoz čerpadla musí být samostatné, vědomé rozhodnutí obsluhy a zachová všechny ostatní dostupné ochrany.

## Stav REV24 (historický audit, aktualizace REV25 níže)

- Uživatel potvrdil rozdělení tří čerpadel: GPIO25/32 na ESP32 #1 a GPIO25 na ESP32 #2.
- V hlavičkách `HardwarePins.h` jsou výstupy pojmenovány podle čerpadel.
- Žádný pump safety GPIO není zatím inicializován ani fyzicky spínán firmwarem.
- `WellPumpProtection` a `UpperTankProtection` zůstávají beze změny.
- Ochrana čerpadla dolní nádrže, její konfigurace a NVS persistence zůstávají otevřené.
- Nebyly změněny žádné hladinové limity, hystereze, tlakové automatiky, nouzové režimy ani řízení ventilů.

## Další implementační pořadí

1. Navrhnout samostatný `LowerTankPumpProtection` využívající validní měření dolní nádrže.
2. Doplnit vlastní konfigurační limity, validaci a NVS ukládání; neodvozovat je z 11cm pracovního minima.
3. Doplnit testy kritické hladiny, obnovovací hystereze, neplatného čidla, restartu, nouzového režimu a systémové chyby.
4. Teprve po ověření všech tří lokálních ochran vytvořit společný fyzický výstupní adaptér s bezpečným stavem při resetu.
5. Provést cílový Arduino build a následné fyzické testy nejprve bez připojených motorů, poté podle bezpečného zkušebního postupu.


## Aktualizace REV25

- `LowerTankPumpProtection` a `NvsLowerTankPumpProtectionStore` jsou implementovány a synchronizovány ve zdrojových stromech.
- Minimum a hysterezi dolního čerpadla nastavuje uživatel; obnova nastává na `minimum + hystereze`.
- Kritický latch a konfigurace se ukládají do NVS. Neplatný senzor, konfigurace nebo úložiště blokuje logické povolení.
- Pracovní minimum 11 cm se nepoužívá jako automatický ochranný práh.
- Fyzické výstupy všech tří pumpových relé zůstávají neaktivní do ověření 24V reléového vstupu, 3,3V triggeru a bezpečného stavu při boot/reset. Uživatel potvrdil přímé přerušení napájení motoru a dostatečné výkonové dimenzování relé.
- Nativní testy 64/64 a ASan/UBSan 64/64; Python 17/17. Arduino build a fyzické testy neprovedeny.
