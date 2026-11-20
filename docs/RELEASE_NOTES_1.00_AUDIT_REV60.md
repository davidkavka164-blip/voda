# Release notes — REV60

- Přidán úplný audit zjednodušení architektury podle potvrzeného rozhodnutí: ESP32 #1 a #2 pracují nezávisle, bez aplikační peer komunikace.
- Popsána odpovědnost ESP32 #1 za transfer horní → dolní podle požadovaných litrů a místního přírůstku objemu dolní nádrže.
- Výslovně potvrzeno hlídání nulového/přerušeného přítoku; parametry budou určeny až podle reálného snímače a průtoku.
- Popsán ruční krokový postup objemové kalibrace horní nádrže v ESP32 #2 podle skutečně přenesených litrů z ESP32 #1; vyžaduje známý absolutní kotevní bod.
- Zjištěno, že REV59 ještě nemá integrovanou objemovou kalibraci horní nádrže ani čtení teploty; GPIO4 je pouze rezervované pro DS18B20.
- Zachovává se W5500 ESP32 #2 pro jeho vlastní síťové připojení; odstraňuje se pouze peer UDP komunikace.
- Aktualizován master dokument na cílovou architekturu 1.4.
- Bez změn provozního firmware, pinů, zapojení, NVS dat ani nastavení.
- REV59 baseline ověřen: 72/72 nativních C++ testů, 17/17 Python testů a statické integrační kontroly PASS. Arduino target build a hardware testy neprovedeny.
