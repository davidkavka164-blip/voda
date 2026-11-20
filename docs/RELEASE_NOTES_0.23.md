# HOME WATER CONTROLLER 0.23

## Nové
- Web ESP32 #1 obsahuje simulační obrazovku transferu mezi studnou/horní nádrží a dolní nádrží.
- Lze nastavit zdroj, požadované litry a timeout, simulaci spustit/zastavit a zobrazit její stav, cílovou hladinu, odhadovaný přírůstek a důvod ukončení.
- Stavový automat se vyhodnocuje podle posledních měření a kontroluje platnost/čerstvost, transferové minimum, kritickou ochranu, kapacitu dolní nádrže a dostupnost peer dat.

## Bezpečnostní hranice
- Jde pouze o SIMULACI rozhodovací logiky. Žádný ventil ani čerpadlo se fyzicky neovládá. Stav `RUNNING` znamená běh simulace, nikoli potvrzení průtoku vody.
- Není dovoleno připojit výkonové výstupy k této revizi jako k hotovému regulátoru.

## Ověření
- 16/16 nativních testovacích programů prošlo.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou s Arduino API stuby.
- Neprovedeno: skutečný Arduino-ESP32 build, webové end-to-end testy na zařízení, W5500/Wi-Fi, NVS a hardware.
