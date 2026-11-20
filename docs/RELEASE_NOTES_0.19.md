# HOME WATER CONTROLLER — vývojová revize 0.19

## Změny proti 0.18
- ESP32 #2 nyní lokálně vyhodnocuje logiku ochrany čerpadla a nečeká na hlavní jednotku. Výpadek peer komunikace proto není jediným místem, které by ochranu rozhodovalo.
- Úložiště ochrany umožňuje samostatný NVS namespace. ESP32 #1 zachovává `hwc_protect`, ESP32 #2 používá `hwc_upperprot`.
- Protože nejsou potvrzeny rozměry horní nádrže a její kritická/obnovovací hladina, konfigurace je záměrně neplatná. Pump permit zůstává blokován a fyzické relé se stále neovládá.

## Ověření
- Nativní testy: 13/13 prošlo; test NVS ochrany ověřuje i oddělený namespace horní jednotky a limit délky namespace.
- Oba sketch soubory prošly `g++ -fsyntax-only` proti hostitelským Arduino API stubům.
- Neprovedeno: skutečný překlad Arduino-ESP32, NVS/W5500/ADC na zařízení a fyzické testy.

## Omezení
Tato revize není připravena k řízení připojených čerpadel nebo ventilů. Dalším krokem je doplnit potvrzenou geometrii a nastavení ochranných mezí horní nádrže, následně ověřit místní ochranu na hardware.
