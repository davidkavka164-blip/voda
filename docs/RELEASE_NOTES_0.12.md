# HOME WATER CONTROLLER — vývojová revize 0.12

## Změny proti 0.11
- Wi-Fi klient nyní opakovaně zkouší obnovit uložené připojení po výpadku.
- Pokud připojení zůstane nedostupné 30 sekund, spustí se konfigurační AP souběžně s opakovanými pokusy o Wi-Fi. AP tak zůstává dostupné i při výpadku routeru.
- Při chybějících Wi-Fi údajích se AP spustí hned; při neúspěšném připojení při startu se AP spustí a klientské připojení se dál zkouší obnovit.
- Web zobrazuje odděleně IP adresu Wi-Fi a AP a stav klientského připojení.
- Přísnější validace číselných polí rozměrů a kalibračních bodů odmítá hodnoty s nečíselnými koncovými znaky a prázdné objemy.
- Přidáno verzované NVS úložiště denní historie přítoku s CRC, validací snapshotu a ochranou před automatickým přepsáním poškozeného záznamu.
- ESP32 #1 načítá a průběžně ukládá 30denní historii; ukládání je omezeno přibližně na jeden zápis za minutu. Po restartu se obnoví historie, ale měřicí baseline se resetuje, aby se neznámý interval během vypnutí nevydával za přítok.
- Čas se vyžádá přes SNTP, pokud je dostupná Wi-Fi. Dokud systémový čas není platný, kladné změny se evidují jako nedatované; místní web nevyžaduje internet.
- Opraveny souhrnné dokumenty, aby rozlišovaly implementaci, nativní testy, překlad a fyzické ověření.

## Kontrola
- Nativní testy: 9/9 testovacích programů prošlo.
- Skutečný Arduino-ESP32 překlad: neproveden; Arduino IDE/toolchain není v tomto prostředí dostupný.
- Wi-Fi/AP, NVS a web na fyzickém ESP32: neověřeno.
- Výkonové výstupy: stále nejsou aktivovány.

## Bezpečnostní poznámka
Web používá HTTP Basic Authentication bez TLS. Používat pouze v důvěryhodné domácí síti nebo zabezpečeném VPN přístupu; nevystavovat přímo do internetu. Hesla vypsaná do Serial Monitoru chránit a po prvním spuštění uložit.
