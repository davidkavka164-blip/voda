# Nouzové ruční ovládání ventilů — implementace REV16

## Stav REV16

REV16 obsahuje pouze softwarovou/logickou vrstvu a webové rozhraní ESP32 #1. Fyzické relé/GPIO nejsou připojena. Implementace fyzických relé navazuje až v REV17.

## Chování

1. Režim je oddělený od nouzového režimu čerpadla (`/pump-emergency`).
2. Aktivace vyžaduje relevantní poruchu/nedostupnost čidla, potvrzení fyzické kontroly hladin, neprobíhající transfer a transferový žurnál ve stavu READY.
3. Aktivace nastaví logický povel `CLOSE_ALL`.
4. Uživatel může zadat jeden ze dvou povolených směrů: studna → dolní nádrž nebo horní nádrž → dolní nádrž.
5. Současně lze mít aktivní jen jednu cestu. Před změnou směru je nutný samostatný STOP/CLOSE_ALL.
6. STOP nastaví CLOSE_ALL a ponechá ruční režim aktivní. Ukončení režimu nastaví CLOSE_ALL a režim deaktivuje.
7. Stav není ukládán do NVS. Po restartu se nepřenáší žádný ruční požadavek; výchozí stav je neaktivní/CLOSE_ALL.
8. Během ručního režimu je blokováno spuštění automatického transferu.

## Bezpečnostní brány v REV16

- Dolní nádrž: čerstvé a platné měření, hladina v rozsahu 0 až pod maximální hranici 121 cm. Při neplatném/stárnoucím měření nebo dosažení maxima jsou oba směry blokovány/ukončeny.
- Studna: vyžaduje platnou konfiguraci ochrany, dostupné úložiště, neaktivní kritickou blokaci a žádnou systémovou chybu. Pokud je měření studny platné, musí být nad stávajícím transferovým minimem. Pokud je neplatné, režim může pokračovat pouze na základě předchozího výslovného fyzického potvrzení; jiné dostupné ochrany zůstávají aktivní.
- Horní nádrž: vyžaduje čerstvá peer data ESP32 #2, shodu limitů a existující povolení `upperValveDecision`. Při ztrátě komunikace se horní cesta nepovolí a již vydaný logický povel se vrátí na CLOSE_ALL.
- Automatický transfer a ruční režim se vzájemně vylučují.
- `pumpProtectionReady` se nepoužívá jako automatická podmínka ručního ventilu.

## Web

Nové endpointy na ESP32 #1:

- `POST /manual-valve-mode` — `mode=enable` s povinným `physical=yes`, nebo `mode=disable`.
- `POST /manual-valve-command` — `command=well|upper|close`.

Všechny endpointy vyžadují stávající webovou autentizaci. Rozhraní zobrazuje výrazné označení nouzového režimu a výslovně uvádí, že povely jsou pouze logické a neprokazují fyzickou polohu ventilu.

## Otevřené body před fyzickou aktivací

- Master 1.3 výslovně zakazuje domýšlet hardware. V REV15/REV16 je GPIO mapa pro ventily označena jako provizorní; jsou uvedeny GPIO 16/17 a reléový modul Drátek 5 V aktivní HIGH, nikoli však ověřená topologie kontaktů a chování celého obvodu.
- Konkrétní typ ventilu, cívka, napájení, NO/NC, monostabilní/bistabilní chování a poloha po výpadku napájení nejsou potvrzeny obsahem Masteru ani zdrojového balíčku.
- Není ověřena zpětná vazba polohy. Web proto nikdy nehlásí fyzické OPEN/CLOSED.
- Není definitivně stanoveno, zda fyzická kontrola obsluhy může nahradit neplatné měření dolní nádrže nebo chybějící peer data horní nádrže. REV16 volí konzervativní blokování.
- Není definitivně stanoveno, zda ruční nouzový režim smí překročit běžný práh `transferValveOpenCm`, nebo zda má být zachován i pro ruční povel. REV16 zachovává stávající `upperValveDecision` a jeho hysterezi; hodnoty ani významy se nemění.
- Před připojením GPIO je nutné ověřit skutečný typ ventilů, schéma relé a napájení, bezpečný stav při resetu/bootu, kontakty, jištění a chování při přerušení vodiče.

## Testy

Stavový automat je hostitelsky testován pro chybějící potvrzení, chybějící poruchu čidla, probíhající transfer, blokovaný žurnál, bezpečnostní zákaz povelu, konflikt směrů, STOP, ukončení a restart. Integrační kontrola potvrzuje, že ESP32 #1 používá endpointy a že v této revizi nejsou reléové GPIO řízeny.
