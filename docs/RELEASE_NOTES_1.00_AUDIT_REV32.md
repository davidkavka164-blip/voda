# Release notes – REV32

## Sjednocení bezpečnostních podmínek OTA

- ESP32 #1 nyní přijímá pouze soubory s příponou `.bin`.
- Aktualizace ESP32 #1 je blokována během výslovně aktivovaného nouzového režimu studničního či dolního čerpadla. Běžné logické povolení čerpadla samo o sobě OTA neblokuje, protože neznamená, že motor skutečně běží.
- Aktualizace ESP32 #1 je blokována během transferu (PRECHECK/RUNNING) a aktivního ručního režimu ventilů.
- Pumpové, transferové a ruční podmínky se kontrolují při zahájení uploadu, před každým datovým blokem a před finalizací. Pokud se stav během uploadu změní, aktualizace se přeruší.
- ESP32 #2 zachovává místní blokaci OTA při povoleném/nouzovém provozu horního čerpadla a znovu kontroluje stav během uploadu.
- Do dokumentace doplněno, že fyzické přepínače pro úplné obejití ESP32 budou samostatným hardwarovým nouzovým stupněm; jejich elektrické schéma zatím není určeno a není součástí firmware.

## Testování

- Nativní testy: 65/65 PASS.
- ASan/UBSan: kompletní sada PASS.
- Python testy: 17/17 PASS.
- Python compileall: PASS.
- Statické integrační kontroly včetně OTA interlocku: PASS.
- ZIP integrita: PASS.
- Arduino-ESP32 cílový build, skutečná OTA aktualizace a fyzické ověření relé/čerpadel: neprovedeno.
