# Audit bezpečnostních relé čerpadel – REV23

## Rozsah auditu

Tato revize prověřuje návaznost stávající softwarové ochrany čerpadel na fyzická bezpečnostní relé. Fyzické výstupy záměrně NEAKTIVUJE. Důvodem není chybějící softwarová logika, ale nedokončené potvrzení elektrického zapojení kontaktů a nejednoznačnost jednoho výstupu v předběžné mapě GPIO.

## Co potvrzuje hlavní dokument 1.3

- ESP32 #1 provádí bezpečnostní zásah do čerpadla studny.
- ESP32 #2 provádí místní ochranu čerpadla horní nádrže.
- Čerpadla běžně řídí vlastní tlaková automatika. ESP32 je nemá běžně spouštět ani zastavovat.
- Při kritické hladině nebo jiném definovaném bezpečnostním důvodu může ESP32 čerpadlo odstavit.
- Nouzový provoz při vadném hladinovém čidle ponechává čerpadlo pod vlastní tlakovou automatikou; zachovají se všechny další dostupné ochrany.

## Zjištěný nesoulad pinové mapy

Předběžná mapa GPIO uvádí na ESP32 #1 dva výstupy `PUMP_1_SAFETY_RELAY` (GPIO25) a `PUMP_2_SAFETY_RELAY` (GPIO32). Hlavní dokument 1.3 však popisuje na ESP32 #1 pouze bezpečnostní zásah do čerpadla studny. Druhý výstup GPIO32 proto nemá v hlavním dokumentu jednoznačně určené čerpadlo ani funkci.

ESP32 #2 má jeden předběžný výstup `PUMP_SAFETY_RELAY` (GPIO25), který odpovídá místní ochraně čerpadla horní nádrže.

Závěr: GPIO25 na ESP32 #1 a GPIO25 na ESP32 #2 jsou kandidátní výstupy pro dvě popsané ochrany. GPIO32 na ESP32 #1 se v této revizi považuje za NEPŘIŘAZENÝ / REZERVNÍ. Nesmí být aktivován, dokud nebude jeho účel potvrzen.

## Logický kontrakt budoucího výstupu

Budoucí výstupní adaptér musí být řízen výhradně výsledkem příslušné lokální ochrany:

- `pumpPermit == true` → povolit běžný provoz tlakové automatiky.
- `pumpPermit == false` → bezpečnostně zakázat provoz čerpadla.
- Po startu/resetu musí být výstup v zakázaném stavu, dokud ochrana neprovede úspěšnou validaci.
- Při kritickém latchi, chybě konfigurace, chybě úložiště nebo neplatném měření bez výslovně aktivovaného nouzového režimu musí být výstup zakázán.
- V nouzovém režimu při poruše hladinového čidla může být `pumpPermit` povolen pouze podle již existujících pravidel ochrany a výslovného potvrzení obsluhy. Nouzový režim nesmí rušit jiné dostupné ochrany.
- Ukončení transferu, zavření ventilu ani dosažení cílového objemu nesmí samo o sobě změnit povolení čerpadla.

To je logický kontrakt. Neurčuje ještě, zda fyzické relé musí být při povolení sepnuté přes NO kontakt nebo jinak; to musí odpovídat ověřenému elektrickému schématu.

## Elektrické podmínky před aktivací

1. Potvrdit, co přesně kontakty každého modulu spínají: nízkonapěťovou povolovací smyčku / vstup tlakové automatiky / cívku stykače, nebo přímo výkonový obvod motoru.
2. Přednostně posuzovat relé jako bezpečnostní zásah do nízkonapěťového řídicího/povolovacího obvodu. Nesmí se předpokládat, že modul s údajem 30 A je vhodný pro přímé spínání motoru; rozhodující jsou motorová/indukční zatížitelnost, rozběhový proud, jištění a příslušné normy.
3. Potvrdit konkrétní kontakt COM/NO/NC a prokázat měřením, který stav znamená povolení a který zákaz čerpadla.
4. Při ztrátě napájení ESP32, 24V reléového modulu nebo při přerušeném vodiči musí být definovaný a ověřený bezpečný stav. Samotné `HIGH`/`LOW` v programu to nedokazuje.
5. Ověřit 3,3V řídicí úroveň ESP32 na vstupu 24V modulu, společnou referenci/galvanické oddělení podle konkrétního zapojení a externí předpětí vstupu při resetu.
6. Ověřit, že výstupní obvod neobchází vlastní elektrické ochrany čerpadla a neovlivní běžné řízení tlakovou automatikou.
7. Zkoušky provádět nejprve bez připojeného motoru a bez výkonového napětí; následně ověřit celý povolovací řetězec kvalifikovaným postupem.

## Stav firmware REV22

- `WellPumpProtection` a `UpperTankProtection` poskytují logické `pumpPermit`.
- ESP32 #1 aktualizuje ochranu studny při měření přibližně po 2 s; ESP32 #2 aktualizuje místní ochranu horní nádrže ve vlastní smyčce přibližně po 2 s.
- ESP32 #2 udržuje místní ochranu nezávisle na peer komunikaci s ESP32 #1.
- Fyzické pump safety výstupy nejsou inicializovány ani ovládány.
- GPIO25/32 jsou pouze předběžné definice pinů. Jejich uvedení v hlavičce neznamená, že je hardware bezpečně zapojen.

## Rozhodnutí REV23

Fyzické výstupy zůstávají deaktivované. Tato revize nepřidává `pinMode`/`digitalWrite` pro pump safety GPIO a nemění žádné hladinové limity, hystereze, NVS ochrany, nouzové chování ani tlakové automatiky.

## Jediný nutný údaj pro další fyzickou integraci

Je nutné potvrdit, zda 24V/30A reléový modul přerušuje **nízkonapěťový povolovací/řídicí obvod čerpadla**, nebo **přímo výkonové napájení motoru**. Současně bude třeba určit, který fyzický kontakt (COM–NO nebo COM–NC) je zapojen tak, aby při bezpečnostním zákazu a při ztrátě napájení byl chod čerpadla znemožněn.
