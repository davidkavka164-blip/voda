# Bezpečná servisní obnova elektrického nastavení snímačů – REV42

Datum: 13. 11. 2026

## Rozsah

Implementace se týká elektrického `SensorConfig` pro studnu a dolní nádrž na ESP32 #1 a horní nádrž na ESP32 #2. Nezasahuje do objemové kalibrace `WellCalibration`, geometrie, ochranných mezí, transfer journalu ani ostatních NVS klíčů.

## Servisní postup

- Samostatná POST cesta `/sensor-recovery` je dostupná jen při chybějícím nebo neplatném elektrickém nastavení.
- Vyžaduje administrátorské ověření, same-origin POST, explicitní fyzické potvrzení, měřítko a skutečnou referenční výšku změřenou nezávislou metodou.
- Obsluha musí zvolit správný snímač. Aktuální elektrický signál se znovu načte v okamžiku požadavku; pokud ADC/signál není platný, obnova se odmítne.
- Z poskytnutého měřítka, surové výšky a nezávislé referenční výšky se vypočítá posun: `offset = reference - rawLevel * scale`.
- Základní elektrické parametry jsou obnoveny z definovaných projektových výchozích hodnot (150 Ω, 4–20 mA, 10 m), nikoli z poškozeného záznamu.
- NVS nedostupné nelze přes webovou cestu obnovit. Platný záznam nelze servisní cestou přepsat.
- Stav snímače se přepne na `VALID_STORED` až po zápisu a úspěšném opětovném načtení/checksum/value verification.
- Před odesláním formuláře obsluha fyzicky změří hladinu nezávislou metodou a potvrdí správné přiřazení snímače. Po uložení stránka zobrazí výsledek a výzvu ke kontrole zobrazené výšky proti referenci; firmware tuto následnou vizuální kontrolu neumí sám ověřit.

## Bezpečnostní chování

Běžné povolení čerpadla/přečerpávání zůstává blokováno do úspěšné obnovy a ověření. Nouzový režim čerpadel se nemění. ESP32 #2 nyní také nezobrazuje provizorní hodnotu jako důvěryhodnou výšku, pokud elektrické nastavení není ověřeno.

## Omezení

- Servisní obnova používá projektové výchozí elektrické parametry shunt/current/range. Pokud instalovaný snímač nebo zapojení používá jiné hodnoty, tato cesta musí být rozšířena o jejich bezpečné zadání; nelze je odhadovat.
- Měřítko musí být předem známo z ověřené vícebodové kalibrace; jeden referenční bod zde dopočítá pouze offset. Pokud je měřítko neznámé, tato servisní cesta nestačí a je nutné nejprve provést samostatnou vícebodovou kalibraci.
- Fyzickou pravdivost referenčního měření nelze ověřit firmwarem. Odpovědnost nese obsluha.
- Neprovedena cílová Arduino kompilace ani fyzická kalibrace na skutečných deskách/snímačích.
