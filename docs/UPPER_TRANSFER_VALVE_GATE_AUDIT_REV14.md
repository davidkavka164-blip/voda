# HOME WATER CONTROLLER — zapojení prahu a hystereze ventilu horní nádrže (REV14)

Datum: 2026-11-10

## Výchozí stav v REV13

`evaluateUpperTankTransferValve()` již vyhodnocoval dvě samostatné hranice:

- `transferValveOpenCm`: práh pro povolení zahájení transferu, pokud ventil dosud není logicky otevřen.
- `householdReserveCm`: spodní hranice hystereze; při již otevřeném ventilu zůstává povolení aktivní pouze nad touto rezervou.

Výsledek `upperValveDecision` se zobrazoval na webu a ovlivňoval `upperValveLogicalOpen`, ale nebyl předáván do `TransferLogic`. Samotný transfer tak mohl být povolen nad rezervou domácnosti, i když ještě nebyl dosažen práh otevření ventilu.

## Provedená změna

1. `TransferInputs` nově nese `upperTransferValvePermitted`.
2. ESP32 #1 předává výsledek stávajícího `upperValveDecision.valid && upperValveDecision.permit`. Pro transfer ze studny se tato podmínka neuplatňuje.
3. `TransferLogic` kontroluje povolení při prechecku před zahájením i během běžícího transferu.
4. Pokud je hladina nad rezervou domácnosti, ale ventilová hystereze transfer nepovoluje, transfer skončí s novým důvodem `TRANSFER_REASON_VALVE_NOT_PERMITTED`.
5. Při dosažení `householdReserveCm` nadále rozhoduje stávající kontrola zdrojového minima (`TRANSFER_REASON_SOURCE_MINIMUM`); logické povolení ventilu se současně zavře podle existující funkce.
6. Nová hodnota enumu byla přidána na konec, aby se nezměnily číselné hodnoty dřívějších důvodů.

Společné soubory `TransferLogic.h/.cpp` jsou synchronizovány v `firmware/common`, `firmware/ESP32_1/src` a `firmware/ESP32_2/src`.

## Hraniční testy

- 59,9 cm při prahu 60 cm a rezervě 35 cm: nový transfer blokován.
- Přesně 60 cm: stávající evaluator povolí otevření a transfer přejde do PRECHECK/RUNNING.
- 45 cm při předchozím logickém otevření: hystereze zachová povolení a transfer pokračuje.
- Přesně 35 cm: evaluator povolení zruší a běžící transfer skončí; ochrana rezervy zůstává účinná.

Testy používají přímo `evaluateUpperTankTransferValve()`, nikoli ručně simulované prahové výsledky.

## Co se nemění

- Žádná hodnota `pumpCriticalTripCm`, `pumpRecoveryCm`, `householdReserveCm`, `transferValveOpenCm` ani `usableDepthCm`.
- Význam kritické hladiny ochrany čerpadla, obnovovací hladiny, rezervy domácnosti a prahu otevření ventilu.
- `pumpProtectionReady` zůstává diagnostický údaj a nestává se podmínkou transferu.
- Nouzové ruční ovládání ventilů se nemění ani touto revizí neimplementuje.
- Fyzické výstupy zůstávají vypnuté; jde o logickou/simulační cestu.

## Ověření a omezení

- Nativní C++ testy a ASan/UBSan: viz release notes REV14.
- Statická integrační kontrola přenosu `upperValveDecision` do `TransferLogic`: přidána.
- Synchronizace společných modulů: ověřena.
- Arduino-ESP32 build, skutečný běh na ESP32, W5500, NVS a fyzické ventily nebyly ověřeny.
