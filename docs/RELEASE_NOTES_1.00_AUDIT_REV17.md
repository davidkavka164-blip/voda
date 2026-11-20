# Release notes — vývojová REV17

## Fyzické relé pro nouzové ruční ovládání ventilů

- Přidán `ValveRelayOutput` pro ESP32 #1.
- GPIO16 je přiřazeno relé cesty studna → dolní nádrž; GPIO17 relé cesty horní → dolní nádrž.
- Relé jsou aktivní HIGH. Inicializace nastaví obě GPIO na LOW ještě před startup delay, sítí a NVS.
- Ruční příkaz sepne pouze příslušné relé; STOP, bezpečnostní blokace, ukončení režimu a restart znamenají obě relé OFF.
- Přechod mezi cestami používá break-before-make; stav se nemění opakovaným zápisem v každém průchodu loop.
- Relé driver je dostupný pouze na ESP32 #1; ESP32 #2 ventily neovládá.
- UI a odpovědi zobrazují vydaný povel, nikoli potvrzenou fyzickou polohu ventilu.

## Rozsah záměrně neprovedených změn

- Automatický transfer zůstává SIMULACE ONLY; TransferLogic není připojena k relé.
- Relé čerpadel zůstávají vypnutá.
- Nemění se žádné hladinové limity, rezervy, kritická ochrana čerpadel, `transferValveOpenCm`, `householdReserveCm` ani hystereze.
- Ruční režim zůstává RAM-only a po restartu se neobnovuje.

## Hardwarové podmínky před fyzickým použitím

- Uživatel potvrdil, že ventily jsou bez napětí zavřené.
- Zapojení cívky musí být přes COM–NO relé, aby vypnuté relé odpojilo napájení cívky.
- Je nutné ověřit 3,3V spouštění 5V relé modulu a doplnit externí pull-down (doporučeně 10 kΩ) na každý aktivně-HIGH vstup relé.
- Je nutné ověřit napětí/proud cívek, zdroj, jištění a ochranu proti indukční špičce.
- Bez polohové zpětné vazby firmware neprokazuje fyzické otevření/zavření.

## Testy

- Nativní testy včetně nového relé driver testu.
- ASan/UBSan včetně nového relé driver testu.
- Statická integrační kontrola: reléové výstupy pouze ESP32 #1, pump relays bez ovládání, ESP32 #2 bez ventilových GPIO.
- Arduino-ESP32 target build a fyzické zkoušky relé/ventilů nejsou provedeny.
