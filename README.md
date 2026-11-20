REV97 rozšiřuje limity HTTP hlaviček na ethernetovém přehledu `/status` a rozlišuje příliš dlouhý a nedokončený požadavek.

Aktualizaci nahraj přes **Systém – aktualizace firmwaru** v ovládacím webu dostupném přes Wi-Fi. Vyber `ESP32_1_REV97.bin` pro jednotku #1 nebo `ESP32_2_REV97.bin` pro jednotku #2. Soubory obsahují samostatný aplikační firmware, sestavený pro ESP32 Dev Module s Arduino-ESP32 1.0.6 a výchozí tabulkou oddílů 4 MB.

Zkoušku proveď s odpojenými výstupy čerpadel a ventilů. Po restartu otevři `/status` na IP adrese W5500 a přihlas se dosavadním účtem admin. Aktualizaci OTA a chování ethernetového webu je potřeba ověřit na desce.
