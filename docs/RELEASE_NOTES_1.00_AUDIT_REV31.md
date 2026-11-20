# Release notes – REV31

## Nouzové povolení všech tří čerpadel při softwarové poruše

- Zpřesněn požadavek uživatele: nouzový režim je určen k zachování dodávky vody i při poruše řídicí automatiky, nikoli pouze při poruše hladinového čidla.
- Nouzové povolení je dostupné pro studniční čerpadlo (ESP32 #1), čerpadlo dolní nádrže (ESP32 #1) i horní čerpadlo (ESP32 #2), a to i při platném měření. Každé čerpadlo má vlastní místní nouzový stav a samostatné webové ovládání.
- Po výslovném potvrzení fyzické kontroly hladiny nouzový režim překonává softwarové blokace ochrany hladiny: neplatné/chybějící/zastaralé měření, kritický latch, chybnou konfiguraci a chybu či nedostupnost NVS. Diagnostické stavy zůstávají zachovány a zobrazují se.
- V běžném režimu zůstává zachována lokální ochrana, kritická mez, hystereze, platnost konfigurace/NVS a požadavek na čerstvé měření (max. 5 s).
- V nouzovém režimu se u příslušného čerpadla obejde i kontrola čerstvosti měření. Výstupní relé však musí být fyzicky inicializované, připravené a povolené commissioning konfigurací.
- Nouzový režim je pouze v RAM, po restartu se vypne. Po ručním ukončení se opět uplatní běžná ochrana.
- Nouzové povolení neobchází nezávislé elektrické jištění, tepelnou/elektrickou ochranu motoru ani mechanickou závadu. ESP32 nemá zpětnou vazbu pro automatickou diagnostiku všech poruch samotného motoru.
- Běžné tlakové spínání čerpadel zůstává samostatné; nouzový režim pouze povoluje bezpečnostní relé motorového napájení.

## Testování

- Nativní testy: 65/65 PASS.
- Kompletní sada ASan/UBSan: PASS.
- Python testy: 17/17 PASS.
- Python compileall: PASS.
- Integrační a statické kontroly: PASS.
- Arduino cílový build a fyzické ověření všech tří relé v jejich skutečných motorových obvodech zůstávají neprovedené.
