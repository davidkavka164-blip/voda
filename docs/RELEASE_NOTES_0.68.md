# HOME WATER CONTROLLER — DEV KIT 0.68

## Oprava diagnostiky ztráty komunikace při transferu

Při transferu z horní nádrže používá ESP32 #1 čerstvá data od ESP32 #2. Po vypršení limitu čerstvosti se zdrojový vzorek zneplatní. Dřívější pořadí kontrol mohlo tento stav označit jako obecnou chybu čidla.

Revize 0.68 přesouvá kontrolu `peerDataValid` před obecnou kontrolu zdrojového vzorku pro zdroj `TRANSFER_SOURCE_UPPER_TANK`. Výpadek spojení je nyní zaznamenán jako `TRANSFER_REASON_COMMUNICATION`.

- Výpadek v PRECHECK: transfer se nespustí.
- Výpadek v RUNNING: simulace se přeruší jako `TRANSFER_ABORTED`; výstupní příkaz je `VALVES_CLOSE_ALL`.
- Transfer ze studny není závislý na peer komunikaci.
- Přidány regresní testy pro oba okamžiky výpadku.

## Ověření

- Nativní testy: 45/45.
- AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- Společný `TransferLogic.cpp` je synchronizován do obou Arduino `src/` složek.
- Arduino-ESP32 překlad a fyzické testy nebyly provedeny.
- Fyzické ventily a čerpadla zůstávají neovládány; jde pouze o simulační automat.
