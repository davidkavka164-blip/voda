# Ruční ovládání ventilů – servisní a nouzový režim REV19

## Cíl

REV19 doplňuje servisní/testovací režim požadovaný Master 1.3 §40. Dosavadní nouzový režim zůstává samostatný. Oba režimy používají existující reléový driver GPIO16/17 a stejné bezpečnostní kontroly; nejde o nový fyzický výstup.

## Servisní režim

- Aktivace vědomým požadavkem z webu, bez fyzického potvrzení hladin, pokud jsou všechna relevantní měření aktuální a platná.
- Pokud je relevantní čidlo vadné nebo data zastaralá, servisní režim se odmítne.
- Otevření cesty studny vyžaduje čerstvé měření studny, platnou konfiguraci ochrany, dostupné úložiště, neaktivní kritickou blokaci a hladinu nad stávajícím transferovým minimem.
- Otevření horní cesty vyžaduje čerstvá peer data, shodu limitů a existující `upperValveDecision` včetně prahu a hystereze.
- Oba směry vyžadují čerstvé a platné měření dolní nádrže pod maximální hladinou.
- Během servisního režimu je automatický transfer blokován. STOP/CLOSE_ALL a ukončení režimu zůstávají dostupné.

## Nouzový ruční režim

- Aktivace je povolena pouze při poruše/nedostupnosti relevantního čidla a vyžaduje explicitní potvrzení fyzické kontroly hladin.
- Fyzické potvrzení může nahradit pouze nedostupné měření studny pro cestu studna → dolní nádrž.
- Neplatné měření dolní nádrže vždy blokuje oba směry. Horní cesta vždy vyžaduje čerstvá peer data ESP32 #2 a existující povolení ventilu.
- Ostatní dostupné ochrany zůstávají zachovány.

## Společná pravidla

- Oba režimy jsou RAM-only; po restartu je ruční ovládání vypnuté a relé jsou v CLOSE_ALL.
- Nelze přímo přepnout směr; před změnou je nutný STOP/CLOSE_ALL.
- Automatický transfer se při aktivním ručním režimu nespustí.
- Reléový driver zůstává aktivní HIGH, napájený 5 V; GPIO16 a GPIO17 jsou řídicí výstupy ESP32.
- Bez fyzické zpětné vazby web zobrazuje povel relé, nikoli skutečnou polohu ventilu.
- Čerpadla se neovládají; nadále používají vlastní tlakovou automatiku.

## Ověření

Hostitelské testy ověřují přepnutí režimů, odmítnutí servisního režimu při poruše čidla, požadavky nouzového režimu, blokaci při transferu/žurnálu, STOP, CLOSE_ALL a stav po restartu. Cílový Arduino build a fyzické zkoušky zůstávají neprovedené.

## Otevřený bod

REV19 zatím neukládá samostatné události aktivace/deaktivace servisního a nouzového ručního režimu do trvalé historie. Master 1.3 §57 tak zůstává pro tyto události nesplněn; nelze je zaměňovat za historii automatických transferů. Samostatné doplnění událostní historie je vhodné provést v navazující revizi s kontrolou kapacity a odolnosti NVS.
