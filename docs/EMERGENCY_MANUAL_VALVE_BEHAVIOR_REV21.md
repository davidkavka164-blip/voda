# Nouzové ruční ovládání ventilů – specifikace REV21

## Kdy je dostupné
Nouzový režim je dostupný pouze při zjištěné poruše relevantního čidla nebo při nedostupné/neaktuální komunikaci s ESP32 #2. Pokud jsou relevantní měření i komunikace v pořádku, nouzový režim se odmítne. Pro plánovaný servis/test se používá oddělený servisní režim.

## Ovládání
1. Obsluha fyzicky ověří hladiny a bezpečnost situace.
2. Aktivuje nouzový režim potvrzením kontroly.
3. Zvolí ZAPNOUT pro jednu trasu: studna → dolní nádrž nebo horní → dolní nádrž.
4. Ventil zůstává ovládacím povelem otevřený bez objemového a časového cíle, dokud obsluha nevydá STOP.
5. STOP/CLOSE_ALL vypne obě relé. Před změnou trasy je povinný STOP.
6. Obsluha průběžně sleduje skutečné hladiny a sama rozhodne o okamžiku zastavení.

## Co systém v nouzovém režimu nedělá
- Nevyžaduje funkční měření dolní ani zdrojové nádrže.
- Pro trasu z horní nádrže nevyžaduje komunikaci s ESP32 #2.
- Nepoužívá cílové litry, cílovou hladinu, rezervu ani automatické ukončení podle hladiny.
- Nepotvrzuje fyzickou polohu ventilu; zpětná vazba polohy není instalována.

## Zbylé blokace a návrat
- Nouzový režim nepřebírá řízení během právě běžícího automatického transferu ani při nevyřešeném restartovém žurnálu.
- Reléový driver musí být inicializován.
- Současně může být aktivní pouze jedna trasa; break-before-make a STOP zůstávají povinné.
- Po restartu jsou ruční režim i otevřený povel vypnuté.
- Po nouzovém zásahu se před návratem k automatice ověří stav čidel a peer komunikace.
