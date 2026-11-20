# HOME WATER CONTROLLER REV95 – stolní ověření ethernetového stavu

ESP32 #1 zpřístupňuje hladinu studny a dolní nádrže přes W5500 na `/` a `/status`.
Přihlášení používá uživatele `admin` a heslo současného Wi-Fi webu. Platné a
čerstvé měření se zobrazí v centimetrech. Při chybě měření nebo chybějící
kalibraci se zobrazí „nedostupné“. Ovládání, servisní formuláře a OTA jsou
dostupné přes stávající Wi-Fi web. Ethernet /status používá HTTP a má vlastní
počítadlo neúspěšných přihlášení podle IP adresy.

- Deska ESP32 Dev Module, Arduino-ESP32 1.0.6, knihovna Arduino Ethernet 2.0.2.
- GPIO W5500: SCK 18, MISO 19, MOSI 23, CS 27, RESET 26. INT nezapojen.
- Síťová úloha vlastní SPI pro W5500, DHCP i TCP. Řídicí smyčka jí předává
  poslední měření a čte stav rozhraní pomocí front bez čekání.
- ESP32 #2 dostala jen úpravy pro sestavení. Ethernetový web je součástí ESP32 #1.

## Stolní zkouška

1. Odpoj výstupy čerpadel a ventilů. V Arduino IDE otevři
   `firmware/ESP32_1/ESP32_1.ino` a nahraj přes USB.
2. Otevři sériový monitor na 115200 baud. Po DHCP se vypíše
   `ESP32 #1 Ethernet status: http://…/status`.
3. Otevři vypsanou adresu z domácí sítě. Prohlížeč má požádat o přihlášení.
   S platným heslem se zobrazí obě hladiny. Bez zapojených zkalibrovaných čidel
   očekávej „nedostupné“. Vyzkoušej i jedno špatné heslo.
4. Vytáhni kabel W5500, znovu zapoj a obnov stránku bez restartu. Pokud router
   přidělil jinou adresu, použij nový výpis. Ověř také dostupnost webu přes Wi-Fi.
