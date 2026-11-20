# HOME WATER CONTROLLER — vývojová revize 0.18

## Změny proti 0.17
- Zpřísněna kontrola hladiny dolní nádrže během transferu: i když snímač vrátí matematicky platnou hodnotu, hodnota mimo fyzický rozsah 0 až 121 cm nyní transfer bezpečně ukončí jako chybu snímače.
- Přidán regresní test pro případ, kdy hladina během transferu přeskočí nad maximální rozsah. Tím se zabrání tomu, aby se takové měření omylem vyhodnotilo jako úspěšné dosažení cíle.

## Kontrola
- Nativní testy a sanitizéry se spustí a výsledky se zapíší do `docs/TEST_AND_REVIEW_LOG.md`.
- Tento balíček není skutečně přeložen Arduino-ESP32 toolchainem a nebyl testován na fyzickém hardware. Výstupy ventilů a čerpadel zůstávají vypnuté.
