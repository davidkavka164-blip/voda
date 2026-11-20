# Release notes – REV30

## Sjednocení nouzového povolení všech tří čerpadel

- Uživatel upřesnil, že chce mít možnost nouzově provozovat čerpadla i bez funkčních hladinových čidel, pokud obsluha fyzicky zkontroluje hladinu.
- REV29 tuto možnost obsahoval pro studniční čerpadlo (ESP32 #1) a horní čerpadlo (ESP32 #2), nikoli pro čerpadlo dolní nádrže.
- REV30 doplňuje samostatný nouzový režim dolního čerpadla na ESP32 #1, ovládaný endpointem `/lower-pump-emergency`. Aktivace vyžaduje čerstvě vyhodnocenou poruchu čidla, potvrzení fyzické kontroly hladiny, platnou bezpečnostní konfiguraci a dostupné NVS.
- Nouzový režim nepřekoná již zaznamenaný kritický latch, neplatnou konfiguraci ani chybu úložiště. Nejde o automatické zapnutí motoru; pouze se dočasně povolí jeho bezpečnostní relé, zatímco běžné tlakové řízení čerpadla zůstává samostatné.
- Nouzový stav je pouze v RAM, po restartu se neobnoví a při ukončení režimu se čerpadlo bez platného měření znovu zablokuje.
- Opraveny zastaralé webové a sériové hlášky, které ještě tvrdily, že fyzická relé čerpadel nejsou aktivní; UI nyní zobrazuje skutečný povel výstupního adaptéru.
- Ochrany studničního, dolního a horního čerpadla zůstávají na sobě nezávislé.

## Audit restartu a bezpečného startu

- Všechny tři pumpové GPIO jsou v `setup()` nastaveny na LOW před startovní prodlevou, síťovou inicializací a načítáním NVS. Teprve po inicializaci výstupního adaptéru a vyhodnocení lokální ochrany může být relé povoleno.
- Nouzový režim studničního, dolního i horního čerpadla je pouze RAM stav; nové instance ochranných tříd po restartu jej neobnovují.
- Nouzový ruční režim ventilů zůstává RAM-only a po restartu se vrací do CLOSE_ALL. Automatický transfer je chráněn persistentním restart journalem a po restartu se neobnovuje bez nového požadavku/obnovovacího potvrzení podle stavu žurnálu.

## Ověření

- Kompletní nativní sada: 65/65 PASS; všechny statické a integrační kontroly PASS.
- Python: 17/17 PASS; `compileall`: PASS.
- Cílené ASan/UBSan testy `WellPumpProtection`, `LowerTankPumpProtection` a `UpperTankProtection`: PASS. Kompletní ASan/UBSan sada byla v tomto běhu přerušena časovým limitem při jiném testovacím programu; není proto označena jako kompletně dokončená.

- Rozšířen `test_lower_tank_pump_protection` o potvrzení fyzické kontroly, odmítnutí bez potvrzení, zákaz obejití kritického latch a fail-closed při chybě konfigurace/úložiště.
- Rozšířeny integrační kontroly dolního čerpadla a nezávislosti tří pumpových ochran.
- Arduino cílový build a fyzické testy všech tří instalovaných relé je nutné provést na cílovém hardware; nejsou nahrazeny hostitelskými testy.
