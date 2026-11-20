# Release notes — vývojová REV18

## Automatický transfer připojen k reléům ventilů

- Existující `ValveRelayOutput` na ESP32 #1 nyní přijímá také příkazy automatické `TransferLogic`.
- Explicitní mapování: studna → dolní nádrž = GPIO16; horní → dolní nádrž = GPIO17.
- Ruční nouzový režim má prioritu; automatický transfer je při jeho aktivaci blokován.
- Automatický start je blokován, pokud reléový driver není inicializován.
- Připravenost reléového driveru je součástí `systemReady` pro transfer.
- Před fyzickým otevřením ventilu musí restart journal úspěšně přejít do ACTIVE. Při chybě zápisu se transfer abortuje a výstup zůstane CLOSE_ALL.
- STOP, dokončení, timeout, neplatná/zastaralá měření, kritická ochrana a ztráta peer komunikace vracejí CLOSE_ALL.
- Při selhání příkazu reléového driveru se transfer abortuje; čerpadla nejsou spínána.
- Webové rozhraní už neoznačuje automatický transfer jako simulaci. Upozorňuje, že bez koncových spínačů a průtokoměru nelze potvrdit polohu ventilu ani průtok.

## Napájení reléového modulu — upřesnění

- Uživatel potvrdil, že reléový modul ventilů je napájen 5 V.
- 5 V se vztahuje k napájení reléové desky, nikoli automaticky k napájení cívek ventilů.
- Vstupy modulu jsou řízeny 3,3V GPIO ESP32; kompatibilita musí být ověřena na konkrétním modulu.
- V pin mapě, dokumentaci a diagnostice je uvedeno 5V napájení reléové desky.

## Co se nemění

- Žádné hladinové limity, rezervy, kritické ochrany, prahy ani hystereze.
- `pumpProtectionReady` zůstává diagnostický údaj.
- Čerpadla se nespínají z ESP32; zůstávají řízena vlastní tlakovou automatikou.
- ESP32 #2 neovládá ventily.
- Při restartu se transfer nikdy automaticky neobnoví.
- Ruční režim zůstává RAM-only a po restartu je vypnutý.

## Testy a limity ověření

- Nativní testy: 61/61 PASS.
- ASan/UBSan: 61/61 PASS.
- Statické/integration skripty: 13/13 PASS.
- Python testy: 17/17 PASS.
- `compileall` pro `tools` a `tests/python`: PASS.
- ZIP archiv: kontrola integrity PASS.
- Arduino-ESP32 cílový build nebyl proveden (Arduino CLI není v prostředí dostupné).
- Fyzické testy 5V reléové desky, 3,3V triggeru, COM–NO, ventilů a hydrauliky nebyly provedeny.
