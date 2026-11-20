# Release notes – REV21

- Nouzový ruční režim lze aktivovat pouze tehdy, když je přítomna skutečná porucha relevantního čidla nebo peer komunikace. Při zdravých čidlech a funkční komunikaci není nouzový režim dostupný; pro test a údržbu zůstává servisní režim.
- Nouzové ovládání je čisté ruční ON/OFF: zvolení směru sepne příslušné relé a ponechá ventil otevřený až do STOP/CLOSE_ALL. Nezadává se objem, cílová hladina ani časový limit a systém neukončuje otevření podle měření.
- Obsluha musí fyzicky sledovat hladiny a sama včas vydat STOP. Po ukončení nouzového zásahu je nutné ověřit čidla před návratem k automatickému provozu.
- Nouzový ON nadále obchází neplatné/chybějící měření dolní a zdrojové nádrže, horní hysterezi a komunikaci s ESP32 #2.
- Zachována blokace převzetí při běžícím transferu a nevyřešeném restartovém žurnálu; nouzový režim nepřerušuje zdravý automatický transfer. Při poruše musí být automatický transfer nejprve ukončen bezpečnostní logikou.
- Zachováno: relay driver ready, pouze jedno aktivní relé, break-before-make, změna směru až po STOP, STOP vždy vypíná obě relé, režim je RAM-only a po restartu vypnutý.
- Bez změny limitů, rezerv, hysterezí, ochran čerpadel, zapojení GPIO16/17 ani 5V napájení reléového modulu.
