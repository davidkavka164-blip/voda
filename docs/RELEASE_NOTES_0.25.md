# HOME WATER CONTROLLER 0.25

## Nové
- Peer status protokol povýšen na verzi 2 a rozšířen z 48 na 52 bajtů o kontrolní součet lokální konfigurace limitů horní nádrže.
- ESP32 #2 pouze oznamuje kontrolní součet již uložených a validních limitů; nepřijímá vzdálené změny a nepřepisuje lokální bezpečnostní konfiguraci.
- ESP32 #1 porovnává kontrolní součty a zobrazuje shodu/nesoulad. Pokud se konfigurace neshoduje, chybí nebo je zpráva stará, simulace transferu z horní nádrže se zablokuje.
- Změna limitů na ESP32 #1 zruší stav shody až do přijetí nové peer zprávy s odpovídajícím kontrolním součtem.

## Bezpečnostní omezení
- CRC a kontrolní součet konfigurace chrání proti náhodnému poškození/nesouladu, nikoli proti úmyslnému podvržení v síti. Nejde o autentizaci.
- Není implementována automatická synchronizace limitů ani řídicí příkazy. Obě jednotky se musí nastavovat místně; neshoda se nyní viditelně odhalí a horní transfer se zablokuje.
- Výstupy zůstávají fyzicky neaktivní.

## Ověření
- Nativní testy 16/16 prošly; PeerProtocol prošel ASan/UBSan; oba sketch soubory prošly hostitelskou syntax kontrolou; ZIP integrita prošla. Výsledky jsou v `TEST_AND_REVIEW_LOG.md`.
- Skutečný Arduino-ESP32 build a fyzický hardware nejsou ověřeny.
