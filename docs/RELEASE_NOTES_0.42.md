# HOME WATER CONTROLLER 0.42

## Časově omezený handshake responder

- Přidán `PeerHandshakeTimeout.*` s wrap-safe výpočtem elapsed času pro monotónní čítač typu `millis()`.
- Timeout musí být v rozsahu 1 až `INT32_MAX` ms; nulová a příliš dlouhá lhůta se odmítá.
- Přidán `TimedPeerHandshakeResponder.*`, který sváže pending HELLO/CHALLENGE s timeoutem, při vypršení zruší pending transcript a odmítne FINISH přijatý v okamžiku timeoutu nebo později.
- Při odmítnutém FINISH se výstupní session key vynuluje. Úspěšný FINISH lze přijmout pouze jednou.
- Testy pokrývají přesnou hranici timeoutu, timeout po wrap-around `millis()`, platné přijetí před lhůtou, nulový timeout, zrušení a opakované FINISH.

## Ověření

- Nativní testovací programy: 26/26 prošlo.
- Celá sada prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Společné moduly synchronizovány do `firmware/ESP32_1/src` a `firmware/ESP32_2/src`.
- Arduino-ESP32 build, W5500 síťová integrace, NVS encryption, provisioning a fyzický hardware nebyly ověřeny.
- Žádné GPIO čerpadel ani ventilů nebylo aktivováno.

## Omezení

Timeout je nyní implementován a nativně testován jako transportně nezávislá vrstva. Není ještě připojen k UDP/W5500 poll smyčce. Volající síťová vrstva musí používat jediný monotónní zdroj času a volat `poll(nowMs)` pravidelně; session smí být povolena až po úspěšném FINISH a následném ustavení `PeerCommandGuard`.
