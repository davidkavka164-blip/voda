# HOME WATER CONTROLLER — DEV KIT 0.70

## Peer sequence rollover a test boot epoch ringu

- `PeerStatusMonitor` nově používá explicitní wrap-safe serial arithmetic pro 32bitovou sekvenci: `0xFFFFFFFF → 0` je platný postup vpřed.
- Převod rozdílu `uint32_t` na `int32_t` byl odstraněn; tím se vyhýbáme implementačně definovanému chování při hodnotách mimo rozsah `int32_t`.
- Rozdíl přesně `0x80000000` je nejednoznačný a je záměrně odmítnut.
- `test_peer_protocol` nově pokrývá přetečení sekvence, opožděný předchozí paket po přetečení, nejednoznačný půlrozsah a kapacitu čtyř položek historie retired boot ID.
- Test výslovně dokumentuje omezení: po pěti změnách epochy nejstarší boot ID vypadne z omezené čtyřpoložkové historie a status-only vrstva jej již nedokáže odlišit od nového boot ID. Tato ochrana je pouze krátkodobá; nenahrazuje autentizaci ani persistentní anti-replay.
- Změněný `PeerProtocol.h/.cpp` je synchronizován bajtově do `firmware/common/` a obou ESP32 `src/`.

## Ověření

- Standardní nativní sada: 45/45.
- Kompletní AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- Statická kontrola stabilní boot identity přes W5500 reconnect: PASS.
- Arduino-ESP32 build ani hardware test nebyl proveden.
- Stavová UDP komunikace zůstává neautentizovaná; fyzické výstupy zůstávají zakázané.
