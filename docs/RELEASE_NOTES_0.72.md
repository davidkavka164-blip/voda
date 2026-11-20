# HOME WATER CONTROLLER — DEV KIT 0.72

## Detekce fyzického výpadku W5500 linky

- Obě ESP32 nyní kromě ztráty DHCP adresy kontrolují `Ethernet.linkStatus()`.
- Při `LinkOFF` se UDP socket zavře, peer transport se označí jako nedostupný a jednotka čeká na obnovení fyzické linky.
- Po přechodu `LinkOFF` → `LinkON` se DHCP inicializace spustí bez čekání na běžný 60sekundový retry interval.
- Při `Unknown` zůstává zachována původní časovaná DHCP retry cesta; to je důležité pro případ, kdy použitý Ethernet řadič/API neumí stav linky určit.
- Boot ID a sekvence ESP32 #2 zůstávají zachovány přes síťové reconnecty.
- Statická regresní kontrola ověřuje obě peer jednotky.

## Závislost

- Funkce `Ethernet.linkStatus()` a hodnoty `LinkON` / `LinkOFF` jsou součástí Arduino Ethernet Library 2.0.0 a novější; fyzické rozpoznání linky je podporováno W5200/W5500. Před překladem je nutné mít kompatibilní verzi knihovny. Zdroj API: https://github.com/arduino-libraries/Ethernet/blob/master/docs/api.md

## Ověření a omezení

- Nativní testy a statické kontroly jsou součástí balíčku.
- Nebyl proveden překlad pro Arduino-ESP32 ani fyzický test odpojení/připojení kabelu na W5500.
- `LinkOFF` detekuje fyzickou linku, nikoli výpadek routeru, VLAN/broadcast izolaci nebo nedostupnost DHCP serveru při zachované ethernetové lince.
- Stavové UDP rámce zůstávají neautentizované a nesmějí být použity jako řídicí kanál.
- Žádné relé, ventily ani čerpadla nejsou aktivovány.
