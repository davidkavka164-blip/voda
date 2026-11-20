# HOME WATER CONTROLLER — revize 0.54

Datum: 1. 11. 2026

## Změny
- Přidán `PeerHandshakeResponderCoordinator` pro transportně nezávislé řízení responder části handshake.
- Replay floor se načítá z `IPeerReplayStore` při zahájení handshake; chyba nebo poškození úložiště vede k odmítnutí.
- Po ověření FINISH vznikne READY, které je cacheováno pro přesné opakované odeslání.
- Session key není dostupný před `markReadyTransmitted()`. Iniciátor READY stále samostatně ověřuje.
- Timeout platí pro čekání na FINISH i pro stav READY_PENDING; chybové ukončení maže READY i session key.

## Ověření
- 40/40 nativních testovacích programů prošlo.
- Nový test responder koordinátoru prošel samostatně AddressSanitizer a UndefinedBehaviorSanitizer.
- 84 souborů `firmware/common` je shodných s oběma firmware `src` složkami.
- ZIP archiv prošel kontrolou `unzip -t`.

## Omezení
- Není implementována UDP/W5500 orchestrace ani síťové retry časování.
- Není implementován bezpečný provisioning dlouhodobého tajemství ani integrace `esp_random()`.
- Handshake není připojen k aktivaci `PeerCommandSession`; řídicí příkazy zůstávají vypnuté.
- Arduino-ESP32 build, skutečné NVS chování při výpadku napájení a fyzický hardware nebyly ověřeny.
