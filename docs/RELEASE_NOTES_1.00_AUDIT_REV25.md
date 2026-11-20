# Release notes — HOME WATER CONTROLLER REV25

## Ochrana čerpadla dolní nádrže

- Doplněna samostatná logická ochrana čerpadla dolní nádrže na ESP32 #1.
- Přidána webová nastavení minimální hladiny a hystereze; obě hodnoty jsou nastavitelné obsluhou.
- Vypnutí nastává při hladině `<= minimum`; obnova až při `>= minimum + hystereze`.
- Pracovní minimum dolní nádrže 11 cm zůstává samostatné a automaticky se nepoužívá jako ochranný práh.
- Konfigurace a kritická blokace se ukládají do NVS pomocí dvouslotového CRC záznamu s kontrolou zápisu.
- Po startu je logické povolení blokováno do platného měření na obnovovací hladině; neplatný senzor, konfigurace nebo NVS blokuje čerpadlo.
- Nouzový režim ventilů ochranu čerpadla nepřepisuje.
- Fyzické pumpové relé výstupy zůstávají deaktivované; REV25 implementuje logiku, konfiguraci a testy, nikoli finální elektrické uvedení motorů do provozu.

## Testy

- Nativní testy: 64/64 PASS.
- Integrační/static testy včetně nové kontroly dolního čerpadla: PASS.
- ASan/UBSan a Python testy se provádějí samostatně; jejich výsledek je uveden v předávacím shrnutí po dokončení běhu.
- Arduino cílový build a fyzické testy: neprovedeny.
