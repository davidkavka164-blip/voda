# Ochrana čerpadla dolní nádrže — REV25

## Účel a oddělení limitů

ESP32 #1 nyní samostatně vyhodnocuje ochranu čerpadla dolní nádrže na základě čidla dolní nádrže (ADC GPIO35). Ochrana je oddělena od:

- informativního údaje 11 cm o běžně využitelné hloubce dolní nádrže (tento údaj nemá žádný řídicí účinek),
- limitů automatického přečerpávání,
- cílového objemu transferu,
- ochrany čerpadla ve studni (GPIO25).

Rozsah dolní nádrže pro kontrolu měření je 0–121 cm. Ochranné nastavení se zadává v centimetrech od dna nádrže. Údaj 11 cm pouze informuje obsluhu o orientační hranici běžného vyčerpání; nesmí blokovat čerpadlo, povolovat čerpadlo ani omezovat transfer.

## Uživatelsky nastavitelné hodnoty

Ochrana používá výhradně dvě řídicí hodnoty: minimální hladinu a hysterezi. Žádné pevné ani skryté pracovní minimum 11 cm do rozhodování nevstupuje.

Webová stránka ESP32 #1 obsahuje sekci **Ochrana čerpadla dolní nádrže**:

1. **Minimální hladina** — při dosažení této hladiny nebo nižší se ochrana kriticky aktivuje a čerpadlo se logicky zakáže.
2. **Hystereze** — hladina musí následně vystoupat alespoň na `minimum + hystereze`, teprve pak se ochrana odblokuje.

Validace vyžaduje minimum ≥ 0 cm, hysterezi > 0 cm a součet nejvýše 121 cm. Hodnoty se ukládají do NVS v samostatném dvouslotovém záznamu s CRC, generací a ověřením zápisu.

**Není nastaven žádný výchozí ochranný práh.** Po instalaci musí obsluha zadat hodnoty podle skutečného čerpadla a jeho sacího uspořádání. Dokud konfigurace není platná a ověřená, logické povolení čerpadla zůstává vypnuté.

## Stavový automat

- Po startu je čerpadlo logicky blokováno do prvního platného měření na úrovni `minimum + hystereze` nebo výše.
- Při hladině `<= minimum` se nastaví kritická blokace.
- V pásmu mezi vypínací a obnovovací hladinou zůstává předchozí stav zachován.
- Při hladině `>= minimum + hystereze` se kritická blokace uvolní.
- Neplatné, zastaralé nebo mimo rozsah vyhodnocené měření znamená zákaz čerpadla.
- Neplatná konfigurace, nedostupné NVS nebo chyba zápisu kritické blokace znamená zákaz čerpadla.
- Kritická blokace se ukládá do NVS. Po restartu se zachová a znovu se ověřuje hladina.
- Nouzový ventilový režim tuto ochranu nepřepisuje. Ochrana čerpadla je samostatná.

## Výstupní hardware

Přiřazení zůstává ESP32 #1 GPIO25 = čerpadlo studny a GPIO32 = čerpadlo dolní nádrže. REV25 doplňuje kompletní logickou ochranu dolního čerpadla a webové nastavení, ale fyzický výstup GPIO32 stále není aktivován. Stejně tak nejsou aktivovány pumpové výstupy GPIO25/ESP32 #1 a GPIO25/ESP32 #2.

Důvodem je oddělení logické validace od finálního uvedení motorových obvodů do provozu. Před aktivací je nutné při odpojených motorech ověřit skutečnou polaritu vstupu relé, COM–NO bezpečný stav, společnou referenci a chování při resetu/bootu. Při návrhu je požadovaný bezpečný stav relé bez napětí = motor odpojen; povolený chod = relé aktivované pouze při platné ochraně.

## Testy

- `test_lower_tank_pump_protection`: startovní blokace, nastavitelné minimum/hystereze, kritická blokace, hysterezní pásmo, obnova, neplatné čidlo, chyba NVS, poškozený checksum a obnovený latch.
- `test_nvs_lower_tank_pump_protection_store`: CRC, dvouslotové ukládání, uchování konfigurace, uložení kritické blokace a fail-closed při přerušeném zápisu.
- `check_lower_tank_pump_protection_integration.sh`: vazba na webové nastavení, NVS, měření a GPIO32; kontroluje, že fyzický výstup zůstává vypnutý.

Hostitelské testy nenahrazují Arduino build ani zkoušku na fyzickém ESP32 a relé.
