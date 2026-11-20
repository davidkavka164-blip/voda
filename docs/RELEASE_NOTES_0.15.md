# HOME WATER CONTROLLER — vývojová revize 0.15

## Změny proti 0.14
- Přidána konfigurace oddělených mezí: transferové minimum studny, kritická hladina pro ochranu čerpadla a obnovovací hladina.
- Konfigurace je verzovaná, kontrolovaná CRC a ukládaná do NVS; po zápisu se znovu načte a ověří.
- Přidáno ukládání a obnovování kritické blokace čerpadla do NVS. Nouzový režim se po restartu neobnovuje.
- ESP32 #1 průběžně vyhodnocuje logickou ochranu studničního čerpadla a zobrazuje její stav na webu. Kritické vypnutí a transferové minimum jsou oddělené; dosažení transferového minima samo o sobě nevypíná čerpadlo.
- Přidáno webové nastavení mezí a nouzový požadavek s povinným potvrzením fyzické kontroly hladiny, pouze při neplatném snímači.
- Při chybě nebo poškození bezpečnostního záznamu se konfigurace automaticky nepřepisuje a logické povolení čerpadla zůstává blokované.

## Kontrola
- Nativní testy: 12/12 testovacích programů prošlo.
- Všech 12 prošlo také s AddressSanitizer a UndefinedBehaviorSanitizer.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům.
- Skutečný Arduino-ESP32 překlad ani fyzický test nebyly provedeny.
- **Fyzické relé čerpadla se stále neovládá.** Implementována je pouze logická vrstva a webové zobrazení; bezpečný stav výstupu musí být ověřen před aktivací.
