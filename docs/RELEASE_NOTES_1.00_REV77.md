# Release notes — DEV KIT 1.00 REV77

Datum: 15. 11. 2026  
Základ: REV76

## Rozsah

REV77 je cílená revize dokumentace a auditu NVS. Firmware a zapojení se nemění. Byly znovu prověřeny zdrojové kódy a testy elektrické kalibrace, ochranných limitů a kritických západek všech tří čerpadel.

## Zjištění

- Elektrická kalibrace: striktní důvěra pouze v ověřený záznam; servisní obnova neumožní přepsat platný záznam, vyžaduje nezávislé fyzické měření a po zápisu invaliduje starý vzorek.
- Studna, dolní čerpadlo a horní safety latch: dvouslotové ukládání, CRC/generace/read-back, fail-closed při poškozeném slotu nebo nejednoznačných generacích. Bez automatického fallbacku/mazání.
- Horní limity: jediný CRC blob s odmítnutím přepsání poškozeného záznamu. To je dostupnostní slabina při přerušení zápisu, ale bezpečnostní výsledek je blokace pumpy. Doporučená budoucí migrace na dvouslotový žurnál.
- Geometrie a objemová kalibrace studny nejsou vstupem do povolení pumpy/transferu.
- Nebyla přidána automatická servisní obnova bezpečnostních limitů ani mazání kritické západky.

## Testy

- `./tests/run_native_tests.sh`: 78/78 nativních testovacích programů PASS; všechny statické integrační kontroly PASS.
- `python3 -m unittest discover -s tests/python -v`: 17/17 PASS.
- ASan/UBSan: 76/76 nativních testovacích programů PASS; statické kontroly obsažené v ASan runneru PASS. První pokus překročil časový limit, opakovaný běh byl dokončen úspěšně.
- Arduino cílový překlad a fyzické testy nebyly provedeny.

## Otevřená práce

1. Migrace `NvsUpperTankLimitsStore` na dvouslotový žurnál s ověřenou migrací původního klíče `state`.
2. Arduino cílový překlad s přesnou verzí core z uživatelova prostředí.
3. Fyzické testy výpadku napájení NVS, čidel a bezpečnostních relé.
