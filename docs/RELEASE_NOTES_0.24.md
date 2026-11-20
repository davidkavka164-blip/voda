# HOME WATER CONTROLLER 0.24

## Nové
- ESP32 #2 má vlastní lokální Wi-Fi AP s autentizovaným webovým rozhraním na `http://192.168.4.1/`.
- Rozhraní umožňuje nastavovat využitelnou hloubku, kritickou/obnovovací hladinu čerpadla, rezervu pro domácnost a hladinu zahájení přepouštění. Uložení ověřuje konfiguraci a kontrolní načtení z NVS.
- Nouzový režim horního čerpadla lze povolit jen při již vyhodnocené chybě snímače a po výslovném potvrzení fyzické kontroly hladiny. Lze jej také výslovně ukončit.
- Nouzový režim se po restartu neobnovuje a nemůže obejít kritickou blokaci, neplatnou konfiguraci nebo chybu NVS. Změna limitů vrací ochranu do startovacího blokování až do dosažení obnovovací hladiny.
- Přihlašovací údaje AP a správce se ukládají do NVS a vypisují se do Serial Monitoru.

## Omezení
- Limity ESP32 #1 a #2 se zatím nesynchronizují automaticky. Musí být ručně nastaveny shodně.
- HTTP Basic Authentication je bez TLS; AP je určeno pouze pro místní správu.
- Žádný fyzický výstup relé/ventilu není aktivován. Nouzové povolení je pouze logické.

## Ověření
- 16/16 nativních testovacích programů prošlo.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou s Arduino API stuby.
- Neprovedeno: skutečný Arduino-ESP32 build, Wi-Fi AP na zařízení, reálné NVS a hardware.
