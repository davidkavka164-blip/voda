# Release notes – REV28

## Pump safety relay outputs enabled

- Na základě potvrzení uživatele převzato funkční zapojení bezpečnostního relé z projektu samostatné studny.
- Použit modul Drátek 24 V DC / 30 A HIGH–LOW, napájení 24 V DC, jumper HIGH a aktivní HIGH řízení z ESP32 GPIO (3,3 V).
- `PUMP_SAFETY_OUTPUTS_ENABLED = true` ve společné pinové mapě i v mapách obou ESP32 sketchů.
- ESP32 #1 GPIO25 ovládá ochranné relé čerpadla studny; GPIO32 relé čerpadla dolní nádrže. ESP32 #2 GPIO25 ovládá relé čerpadla horní nádrže.
- HIGH = motor povolen; LOW = relé odpadlé a napájení motoru přerušené.
- Výstupy se inicializují do LOW na začátku `setup()` a poté se řídí lokálním `pumpPermit` a čerstvostí příslušného měření (max. 5 s).
- Nezměněny ochranné prahy, hystereze, NVS konfigurace, tlakové automatiky ani nouzová logika.
- Ochrany jsou nezávislé: studna a dolní nádrž používají na ESP32 #1 oddělené senzory, stavy i výstupy; horní nádrž vyhodnocuje ochranu místně na ESP32 #2 bez závislosti na peer komunikaci.
- Nouzový režim ventilů obchází pouze běžné podmínky transferu/hladin ventilového procesu. Neobchází platnou ochranu zdrojového čerpadla; při vadném snímači může obsluha po fyzické kontrole nouzově otevřít ventil, ale samotné čerpadlo zůstává blokováno, dokud není samostatně povolen jeho nouzový režim.
- Nouzový režim ochrany čerpadla je oddělený od nouzového ovládání ventilů a vyžaduje samostatné fyzické potvrzení. Kritická blokace, chyba konfigurace nebo chyba úložiště jej nadále nepřekonají.

## Verification

- Hostitelské testy ověřují logiku výstupního adaptéru a propojení všech tří ochranných stavů.
- Arduino cílový build a fyzické zkoušky všech tří osazených relé v motorových obvodech nebyly v této revizi provedeny.
