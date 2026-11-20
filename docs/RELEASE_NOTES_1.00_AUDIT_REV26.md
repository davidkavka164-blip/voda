# RELEASE NOTES — HOME WATER CONTROLLER 1.00 AUDIT REV26

## Upřesnění významu 11 cm v dolní nádrži

Uživatel výslovně upřesnil, že 11 cm není řídicí minimum. Jde pouze o informativní údaj, který obsluze ukazuje orientační hranici běžně využitelného vyčerpání dolní nádrže.

### Závazná řídicí pravidla

- Ochrana čerpadla dolní nádrže používá pouze dvě uživatelsky nastavitelné hodnoty: **minimální hladinu** a **hysterezi**.
- Při hladině `<= minimum` se aktivuje kritická blokace čerpadla.
- Opětovné povolení nastane až při hladině `>= minimum + hystereze`.
- V hysterezním pásmu zůstává předchozí stav zachován.
- Pevná hodnota 11 cm nesmí ovlivnit povolení/blokování čerpadla, automatický transfer ani ruční nouzové ovládání.
- 11 cm zůstává pouze zobrazovanou informací o běžně využitelné hloubce. V UI není prezentována jako bezpečnostní varování ani jako blokovací limit.

### Změny v REV26

- Webové rozhraní označuje 11 cm jako informativní údaj.
- Popis ochrany čerpadla výslovně uvádí, že ji řídí výhradně nastavené minimum a hystereze.
- Dokumentace požadavků a implementační plán byly sjednoceny s tímto pravidlem.
- Přidán statický integrační test, který hlídá, že 11 cm není použito v logice ochrany čerpadla.
- Logika `LowerTankPumpProtection` se funkčně nemění; již používá jen uložené `minimumLevelCm` a `hysteresisCm`.

### Rozsah a ověření

REV26 je zpřesnění dokumentace a uživatelského rozhraní, nikoli změna prahové logiky. Předchozí ochranné chování, NVS formát, nastavení uživatele, reléové výstupy ani limity transferu se nemění.

Arduino cílový build a fyzické ověření ESP32/relé zůstávají neprovedené.
