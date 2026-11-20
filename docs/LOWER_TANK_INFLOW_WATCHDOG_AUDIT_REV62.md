# Audit detektoru nulového/přerušeného přítoku — REV62

## Co bylo doplněno

Byl vytvořen přenosný modul `LowerTankInflowWatchdog` ve společné části a zrcadlen v obou sketchích ESP32. Modul přijímá objem dolní nádrže, platnost vzorku a explicitní konfigurační parametry:

- `startupGraceMs` — nejdelší rozběhové okno pro první měřitelný přírůstek;
- `noProgressTimeoutMs` — maximální doba bez dalšího dostatečného přírůstku po potvrzení průtoku;
- `minimumProgressLiters` — minimální přírůstek nad šum/rozlišení čidla.

Výstavy jsou `WAITING_FOR_FIRST_PROGRESS`, `CONFIRMED`, `NO_PROGRESS`, `INVALID_SAMPLE` a `INVALID_CONFIG`. Chybový stav je koncový až do resetu/nového `begin()`. Časové rozdíly používají unsigned aritmetiku, takže jsou bezpečné přes přetečení `millis()`.

## Záměrná omezení

- Modul zatím není zapojen do `TransferLogic` ani do provozní smyčky ESP32 #1.
- Nejsou stanoveny provozní hodnoty prahů. Testy používají pouze syntetické hodnoty pro ověření stavového automatu; nejsou to doporučená nastavení pro reálnou nádrž.
- Před integrací je nutné změřit šum a rozlišení dolního snímače, interval jeho aktualizace a skutečný průtok.
- Souběžný odběr z dolní nádrže může přírůstek maskovat. První verze proto musí během transferu zakázat souběžný odběr, nebo bezpečně přerušit transfer při zjištění, že měření nelze spolehlivě interpretovat.
- Neplatný nebo zastaralý vzorek musí vést k uzavření všech ventilů; samotný watchdog nezajišťuje fyzické uzavření.

## Testy

Nový test `test_lower_tank_inflow_watchdog.cpp` pokrývá neplatnou konfiguraci, nulový počáteční přítok, opožděný první přírůstek, potvrzený průtok, následné přerušení, šum/pokles hladiny, neplatný vzorek a přetečení `millis()`.

Modul byl zahrnut do běžné nativní testovací sady i ASan/UBSan sady. Úspěch těchto testů potvrzuje pouze přenosný stavový automat, nikoli integraci s relé/ventily, cílový Arduino překlad ani fyzický průtok.
