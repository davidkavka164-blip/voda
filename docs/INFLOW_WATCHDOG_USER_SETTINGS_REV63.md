# Nastavení ochrany přítoku — REV63

## Účel

Časové limity ochrany nulového a přerušeného přítoku budou nastavitelné přímo na webu ESP32 #1. Uživatel je doladí při provozním měření.

## Parametry

| Interní parametr | Popisek ve webu | Význam |
|---|---|---|
| `startupGraceMs` | Čas na zahájení přítoku (s) | Doba od otevření ventilu do prvního měřitelného přírůstku |
| `noProgressTimeoutMs` | Nejdelší přerušení přítoku (s) | Doba bez dalšího dostatečného přírůstku po potvrzení přítoku |
| `minimumProgressLiters` | Minimální přírůstek (l) | Mrtvé pásmo proti šumu/rozlišení čidla; není časovým limitem |

## Chování

- Nastavení se provádí v chráněné webové stránce ESP32 #1.
- Časy se zadávají v sekundách; firmware je bezpečně převede na milisekundy.
- Uložené hodnoty se uchovají v NVS a při startu se načtou.
- Zápis se ověří zpětným načtením.
- Chybějící, poškozená nebo mimo rozsah uložená konfigurace blokuje automatický transfer horní → dolní; nesmí se tiše použít neověřená hodnota.
- Změna při běžícím transferu se projeví až při dalším transferu.
- Celkový timeout transferu zůstává nezávislý a dál se zadává u konkrétního požadavku.
- Počáteční výchozí hodnoty budou označené jako neověřené. Uživatel je doladí podle skutečného průtoku a odezvy hladinového čidla.

## Testy před zapojením do provozu

1. Uložit platné časy a ověřit jejich načtení po restartu.
2. Odmítnout prázdné, nulové, záporné, nečíselné a mimo rozsah zadané hodnoty.
3. Simulovat poškozený NVS záznam; horní transfer musí zůstat blokován.
4. Změnit hodnoty během aktivního transferu; aktuální transfer musí používat původní kopii.
5. Ověřit samostatně rozběhový limit, limit bez pokroku a celkový timeout transferu.
6. Ověřit přetečení `millis()` a převod sekund na milisekundy.

## Stav REV63

Toto je schválená specifikace. NVS úložiště, webový formulář a integrace watchdogu zatím nejsou implementované.
