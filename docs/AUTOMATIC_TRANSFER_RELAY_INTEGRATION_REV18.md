# Automatický transfer přes fyzická relé — REV18

## Cíl změny

REV18 připojuje existující `TransferLogic` k již zavedenému `ValveRelayOutput` na ESP32 #1. Stejná dvě relé nyní ovládají automatické i nouzové ruční cesty. Nejde o nové výstupy ani změnu pin mapy.

| Cesta | GPIO ESP32 #1 | Stav relé při otevření | Stav relé při CLOSE_ALL |
|---|---:|---|---|
| Studna → dolní nádrž | GPIO16 | HIGH / sepnuto | LOW / vypnuto |
| Horní nádrž → dolní nádrž | GPIO17 | HIGH / sepnuto | LOW / vypnuto |

Reléová deska je napájena **5 V**. To je napájení modulu relé, nikoli údaj o napětí cívek ventilů. Řídicí signál z ESP32 je 3,3 V; jeho spolehlivé rozpoznání konkrétním 5V modulem musí být ověřeno měřením. Ventily jsou uživatelem potvrzeny jako zavřené bez napětí. Cívky proto musí být spínány přes COM–NO, aby vypnuté relé odpojilo jejich napájení.

## Tok automatického povelu

1. Webový požadavek je odmítnut, pokud je aktivní nouzový ruční režim nebo není inicializován reléový driver.
2. Před zahájením se ověří připravenost restart journalu a historie; požadavek se uloží jako PREPARED.
3. `TransferLogic` provede dosavadní PRECHECK. V této fázi zůstávají obě relé vypnutá.
4. Při přechodu do RUNNING se nejprve zapíše ACTIVE do restart journalu. Pokud zápis selže, transfer se ukončí a ventilový povel zůstane CLOSE_ALL.
5. Teprve výstup RUNNING se explicitně převede na příkaz `ValveRelayOutput`. Před sepnutím jedné cesty driver vypne obě relé (break-before-make).
6. STOP, dosažení cíle, timeout, chyba měření, ztráta komunikace s ESP32 #2, ztráta povolení horního ventilu nebo jiná bezpečnostní podmínka vrací `VALVES_CLOSE_ALL`.
7. Výsledek se uloží do historie a restart journalu podle dosavadního postupu. Při restartu se aktivní transfer nikdy automaticky neobnovuje.

## Zachované bezpečnostní podmínky

- Samostatná čerstvost a validita měření studny a dolní nádrže.
- Ověření rozsahů hladin, cílové kapacity dolní nádrže a časového limitu.
- Ochrana zdroje studna podle stávající konfigurace, kritické blokace a stavu úložiště.
- Pro horní nádrž čerstvá peer data, shoda uložených limitů a existující `upperValveDecision` včetně `transferValveOpenCm` a hystereze.
- Změna boot epoch ESP32 #2 během horního transferu přenos ukončí; nová komunikace sama transfer neobnoví.
- `pumpProtectionReady` zůstává diagnostický údaj a nebyl přidán jako nová blokovací podmínka.
- Ruční režim má prioritu a automatický transfer při něm nelze spustit.
- Relé čerpadel zůstávají mimo řízení firmwaru. Otevření ventilu samo o sobě není příkazem ke spuštění čerpadla; čerpadla nadále řídí vlastní tlaková automatika.
- ESP32 #2 ventily neovládá.

## Význam stavů a měření

Web zobrazuje požadovaný/předaný příkaz relé, nikoli ověřenou fyzickou polohu ventilu. Nejsou přítomny koncové spínače ani průtokoměr. Odhad přeneseného objemu se nadále počítá ze změny hladiny dolní nádrže; nelze jej vydávat za přímé měření průtoku. Případný nefunkční ventil, zavzdušnění, porucha čerpadla nebo hydraulická překážka proto nemusí být samotným řízením relé rozpoznána.

## 5V reléová deska — hardwarové podmínky

- Napájení modulu relé: 5 V se správnou proudovou rezervou.
- Společná zem řídicího signálu podle zapojení konkrétní desky; pokud má deska opticky oddělené vstupy nebo oddělené napájecí propojky, zapojení se musí řídit jejím schématem.
- Ověřit, že 3,3V HIGH z ESP32 spolehlivě sepne vstup 5V reléového modulu.
- Na každý aktivně-HIGH vstup IN doplnit externí pull-down (doporučeně 10 kΩ) k zajištění vypnutého stavu během resetu/bootu, kdy jsou GPIO ve vysoké impedanci.
- Cívky ventilů napájet jejich vlastním jmenovitým napětím a proudově dimenzovaným zdrojem; nepředpokládat 5 V podle napájení reléové desky.
- Ověřit COM–NO, jištění, průřezy, ochranu proti indukční špičce a bezpečné oddělení napájecích obvodů.
- Nejprve provést zkoušku relé bez ventilů a vody, včetně resetu, výpadku 5 V a přerušení řídicího vodiče. Poté zkoušku ventilů bez tlaku/průtoku.

## Omezení ověření REV18

Hostitelské testy ověřují software a statickou integraci. Nebyl proveden Arduino-ESP32 cílový build ani fyzická zkouška ESP32, 5V reléové desky, ventilů, čerpadel nebo hydrauliky. Před reálným provozem je proto nutné provést výše uvedené hardwarové kontroly a řízený test systému.
