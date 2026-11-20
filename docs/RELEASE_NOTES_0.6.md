# HOME WATER CONTROLLER — vývojový balíček 0.6

## Změny
- Potvrzen modul relé ventilů Drátek.cz „Modul relé 5V – 1 kanál s úrovňovým spínačem“, produkt 1517435132.
- Pro obě reléové varianty je v pracovním návrhu zvolen režim HIGH.
- Doplněny hardwarové položky a bezpečnostní poznámky do `GPIO_DRAFT.md`, `OPEN_HARDWARE_ITEMS.md` a `IMPLEMENTATION_STATUS.md`.
- Aktualizován společný `HardwarePins.h` a synchronizován do obou Arduino sketch adresářů.

## Omezení
- Žádné fyzické výstupy nejsou aktivovány.
- Neověřeno na skutečném ESP32: 3,3V kompatibilita vstupů relé, reset/boot stavy, typy ventilů a zapojení bezpečnostních kontaktů čerpadel.
- Arduino kompilace a test na hardware nebyly provedeny.
