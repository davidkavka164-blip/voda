# HOME WATER CONTROLLER — vývojová revize 0.14

## Změny proti 0.13
- Přidán základní OTA upload firmwaru pro ESP32 #1 přes webové rozhraní s ověřením správce.
- Během uploadu se kontroluje výsledek zápisu; při chybě se aktualizace neaktivuje a zařízení se nerestartuje. Při úspěšném dokončení se ESP32 #1 restartuje.
- Aktualizace používá mechanismus `Update` a nemaže NVS; funkčnost ale závisí na tabulce oddílů obsahující OTA partition a na velikosti výsledného binárního souboru.
- Web výslovně uvádí, že OTA ESP32 #2 ještě není implementováno.

## Kontrola
- Nativní logické testy z předchozí revize zůstávají součástí balíčku.
- Oba `.ino` soubory prošly hostitelskou syntaktickou kontrolou proti dočasným Arduino API stubům; není to skutečný Arduino build.
- OTA nebylo otestováno na zařízení ani s reálným `.bin` souborem.
- Výkonové výstupy zůstávají vypnuté.
