# Release notes – REV29

## Audit nezávislosti ochran čerpadel a sjednocení dokumentace

- Provedena kontrola všech tří ochranných větví: studna (ESP32 #1 / GPIO25), dolní nádrž (ESP32 #1 / GPIO32) a horní nádrž (ESP32 #2 / GPIO25).
- Každý fyzický výstup používá vlastní lokální `pumpPermit` a čerstvost vlastního měření. Porucha snímače, konfigurace nebo úložiště jedné ochranné větve není použita jako blokace ostatních nezávislých větví.
- Horní čerpadlo je chráněno lokálně na ESP32 #2; jeho reléové povolení nezávisí na peer komunikaci s ESP32 #1.
- Nouzové ruční ovládání ventilů zůstává oddělené od nouzového režimu ochrany čerpadla. Při platném měření studny musí ventilová cesta respektovat studniční `pumpPermit`; při vadném čidle může obsluha po fyzické kontrole nouzově otevřít ventil, ale samotné čerpadlo zůstává blokováno, pokud není samostatně potvrzen nouzový režim jeho ochrany.
- Nouzový režim ochrany čerpadla nepřekoná kritický latch, chybnou konfiguraci ani chybu úložiště. Dolní čerpadlo v REV29 ještě nemělo nouzový bypass; tento rozdíl je napraven v REV30.
- Z README a aktuálních pinových poznámek odstraněna zastaralá tvrzení, že pumpová relé jsou stále neaktivní, a požadavek znovu ověřovat již potvrzené pumpové zapojení. Samostatné ověření 5V reléových modulů ventilů zůstává otevřené.
- Přidán `check_pump_protection_independence.sh`, který kontroluje nezávislost permitů, lokální ochranu horního čerpadla a oddělení nouzového ventilového režimu od ochrany čerpadel.

## Testy

- Nativní testy: 65/65 PASS.
- ASan/UBSan: 65/65 PASS.
- Python testy: 17/17 PASS.
- `compileall`: PASS.
- Všechny statické/integrace kontroly včetně nového testu nezávislosti: PASS.
- Arduino cílový build nebyl proveden (Arduino CLI není v prostředí dostupné).
- Fyzické testy tří instalovaných relé v tomto společném projektu nebyly provedeny; známé funkční zapojení je převzato ze samostatného projektu studny.
