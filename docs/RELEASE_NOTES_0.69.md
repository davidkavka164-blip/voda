# HOME WATER CONTROLLER — DEV KIT 0.69

## Oprava peer boot epochy a kontrola návaznosti s MASTER 1.3

- `peerBootId` na ESP32 #2 se nově inicializuje jednou v `setup()`.
- Opakované pokusy o DHCP/W5500 již nemění `peerBootId` ani nevynulují `peerSequence`.
- Při skutečném restartu vznikne nová boot epocha; při reconnectu pokračuje stávající sekvence.
- `PeerStatusMonitor` odmítá opožděné pakety z posledních čtyř již opuštěných boot epoch; přidán regresní test návratu opožděného UDP rámce.
- Přidán `docs/MASTER_ALIGNMENT_AUDIT_0.69.md`, který porovnává DEV KIT 0.66, 0.67 a 0.68 se závazným MASTER 1.3 a uvádí skutečný stav implementace i otevřené blokátory.
- Historický audit 0.60 zůstává zachován jako historický záznam, nikoli jako aktuální matice.
- Oprava nemění fyzické výstupy. Stavová komunikace zůstává neautentizovaná a nesmí být použita pro fyzické řízení.

## Ověření

- Nativní testy: 45/45.
- AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- Statická kontrola: `peerBootId = esp_random()` je pouze v jednorázové inicializaci; `startPeerEthernet()` již boot ID ani sekvenci nemění.
- `PeerProtocol` odmítne opožděný paket z nedávno opuštěné boot epochy; test je součástí `test_peer_protocol`.
- Arduino-ESP32 build a hardware nebyly ověřeny.
