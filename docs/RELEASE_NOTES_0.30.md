# HOME WATER CONTROLLER 0.30

## Oprava nezávislé čerstvosti měření pro transfer
- Při revizi transferového UI ESP32 #1 bylo zjištěno, že společná podmínka `samplesFresh` vyžadovala současně existující a čerstvé měření studny i dolní nádrže pro oba směry transferu.
- Důsledek: chyba/nepřítomnost snímače studny mohla blokovat transfer z horní nádrže, přestože horní zdroj i dolní cílová nádrž měly platná měření. Analogicky neplatný snímač nesouvisejícího zdroje zbytečně zneplatňoval měření cíle.
- Přidán `SampleFreshness.h` s wrap-safe funkcí pro nezávislou kontrolu dostupnosti a stáří jednotlivého vzorku.
- ESP32 #1 nyní vede samostatné časové značky posledního měření studny a dolní nádrže. Platnost cílové dolní nádrže se vyhodnocuje nezávisle; zdroj se vyhodnocuje podle zvoleného směru (studna nebo horní nádrž).
- Přidán `test_sample_freshness` včetně hranice stáří, chybějícího vzorku, nezávislosti senzorů a přetečení `millis()`.
- Fyzické výstupy zůstávají odpojené; funkce transferu je stále pouze simulace.

## Ověření
- Nativní testovací programy: 18/18 prošlo.
- Nový test čerstvosti je přenosný C++11 test; skutečný Arduino-ESP32 build nebyl proveden.
- ASan/UBSan: provedena kontrola celé nativní sady.
- Společné soubory synchronizovány do obou sketch `src` adresářů.
- Skutečný Arduino build, W5500, Wi-Fi a fyzické senzory/relé/ventily nebyly testovány.
