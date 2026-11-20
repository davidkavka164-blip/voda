# Revize 0.32 — opakování ukládání historie transferové simulace

## Nalezený problém
Kontrola integrace historie v revizi 0.31 odhalila, že terminální záznam se označil jako zpracovaný ještě před úspěšným uložením do NVS. Pokud `append()` proběhl, ale NVS nebylo dostupné nebo `save()` selhalo, záznam zůstal pouze v RAM a další pokus už nenastal. Stejně tak se nedostupné NVS po startu zkoušelo pouze jednou.

## Změna
- Terminální záznam se po úspěšném přidání do RAM označí jako čekající na trvalé uložení.
- Neúspěšné NVS uložení ponechá historii označenou jako dirty; další pokus se provádí nejdříve po 5 sekundách, aby nedocházelo k zápisu v každém průchodu `loop()`.
- Pokud NVS při startu není dostupné, ESP32 #1 se pokusí znovu otevřít úložiště každých 30 sekund.
- Po pozdějším zpřístupnění NVS se nejprve načte a ověří existující historie. Pokud je záznam poškozený, automatický zápis zůstane zakázán.
- Záznamy vzniklé mezitím v RAM se sloučí s načtenou historií; duplicity podle `requestId` se nepřidají znovu.
- Pokud NVS obsahuje platnou historii, její starší záznamy se zachovají a čekající RAM záznamy se doplní do kruhového bufferu.

## Bezpečnostní rozsah
- Jde stále výhradně o historii SIMULACE transferu. Nejde o potvrzení skutečného průtoku ani o fyzické řízení ventilů.
- CRC32 chrání integritu uloženého záznamu proti náhodnému poškození, nikoli proti úmyslné manipulaci.
- Žádné GPIO čerpadel nebo ventilů nebylo aktivováno.

## Ověření
- Nativní testovací sada: 20/20 prošlo.
- Celá nativní sada prošla AddressSanitizer a UndefinedBehaviorSanitizer.
- Hostitelská syntaktická kontrola `.ino` v této revizi nebyla provedena.
- Skutečný Arduino-ESP32 build, NVS na fyzickém ESP32 a hardware nejsou ověřeny.
