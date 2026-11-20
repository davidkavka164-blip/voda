# HOME WATER CONTROLLER — REV62

- Přidán přenosný modul `LowerTankInflowWatchdog` pro detekci nulového a přerušeného přítoku podle přírůstku objemu dolní nádrže.
- Modul je zrcadlen v common a zdrojích obou ESP32.
- Přidán nativní test včetně testu přetečení `millis()`; modul je součástí běžné i ASan/UBSan testovací sady.
- Aktualizován MASTER na 1.6.
- Provozní firmware, síťová komunikace, piny, relé a transferová logika zatím nebyly změněny. Watchdog není integrován a jeho provozní prahy nejsou nastaveny.
