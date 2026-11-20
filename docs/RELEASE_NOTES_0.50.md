# HOME WATER CONTROLLER 0.50

## Změny
- Přidán `PeerSequenceAllocator` pro monotónní sekvence odchozích COMMAND rámců.
- `PeerCommandSession::prepareOutgoingRequest()` automaticky přidělí sekvenci a uloží do korelační tabulky přesně odesílaný rámec.
- Přidána `synchronizeOutgoingSequenceFloor()`: po autentizovaném předání persistentní hranice přijímače posune další sekvenci na `floor + 1`; nižší hranice čítač nikdy nevrátí zpět.
- Reconnect se stejným lokálním boot ID zachová čítač. Nové lokální boot ID zahájí novou sekvenční epochu. Vyčerpání rozsahu `< 2^31` blokuje další požadavky.
- Integrační test persistentního anti-replay nyní ověřuje synchronizaci floor 64 a odeslání sekvence 65 po restartu přijímače.

## Ověření
- 37/37 nativních testovacích programů prošlo.
- Nový allocator a upravený integrační test prošly AddressSanitizer + UndefinedBehaviorSanitizer.
- Společné zdrojové soubory jsou synchronizovány do obou Arduino `src` složek.
- ZIP prošel `zip -T`.

## Omezení
- Metoda `synchronizeOutgoingSequenceFloor()` předpokládá, že její argument pochází z autentizované komunikace. Sama autentizaci neprovádí.
- HELLO/CHALLENGE/FINISH zatím persistentní hranici nepřenáší; W5500 řídicí transport, retry a transferová integrace nejsou hotové.
- NVS backend je otestován hostitelským stubem; fyzický power-cut test a překlad Arduino-ESP32 nebyly provedeny.
- Čerpadla a ventily zůstávají softwarově neaktivní.
