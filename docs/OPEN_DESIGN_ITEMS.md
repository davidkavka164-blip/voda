# Otevřené návrhové otázky zjištěné při implementaci

## 1. Absolutní objem studny versus změna objemu

Transfer do dolní nádrže poskytne měřený objem přenesené vody a tím změnu objemu studny, pokud během měření nedochází k významnému přítoku ani odběru pro domácnost. Samotné rozdíly ale neurčují absolutní množství vody ve studni: pro absolutní objem je nutný alespoň jeden známý referenční bod (např. známý objem při konkrétní hladině). Bez něj lze spolehlivěji pracovat s relativními změnami a denním přítokem, ale nelze vydávat absolutní zásobu za přesnou.

Bezpečný návrh: křivka bude mít explicitní stav `ANCHOR_KNOWN` / `RELATIVE_ONLY`. Dokud neexistuje platný kotevní bod, web může ukazovat hladinu a změny objemu, ale absolutní objem studny musí být označen jako relativní/neověřený.

## 2. Lokální překalibrace a zachování starých úseků

Požadavek zachovat staré body mimo překalibrovaný úsek znamená, že na hranicích nové oblasti musí být vyřešena návaznost nové a staré křivky. Jestliže nová měření neodpovídají staré křivce na obou hranicích, nelze současně přesně zachovat starou křivku mimo úsek a bez dalšího přijmout všechny nové absolutní hodnoty uvnitř. Bezpečné řešení musí kontrolovat návaznost a při nesouladu zobrazit rozdíl a neukládat potichu nespojitý výsledek.

## 3. Přesnost kalibrace při souběžném odběru/přítoku

Změna hladiny studny během transferu neodpovídá přesně přeneseným litrům, pokud ve stejnou dobu do studny přitéká voda nebo čerpadlo dodává vodu do domu. Kalibrační režim proto musí upozornit, že souběžný odběr/přítok může bod zkreslit, a umožnit uživateli bod zrušit. Běžné transfery křivku nikdy automaticky nemění.

## 4. Bezpečnostní výstupy

Software může určit požadovaný bezpečný stav, ale skutečné bezpečné chování závisí na elektrickém zapojení: aktivní úrovni relé, typu ventilu (NO/NC, monostabilní/bistabilní), výstupním modulu a chování při resetu či výpadku napájení. Dokud toto není potvrzeno, kostra nesmí řídit fyzické výstupy.

## 5. Hladinové meze studny — musí být nastaveny před použitím
- Web nyní umožňuje nastavit odděleně transferové minimum, kritickou hladinu ochrany čerpadla a obnovovací hladinu.
- Žádné z těchto tří čísel není potvrzeno jako skutečná hodnota pro instalaci. Dokud nejsou nastaveny a ověřeny podle fyzické studny a čidla, logické povolení čerpadla zůstává blokované.
- Kritická blokace se ukládá do NVS; nouzový režim při vadném snímači se po restartu neobnovuje.
- I po zadání mezí zůstává fyzické relé vypnuté v kódu, dokud nebude ověřen bezpečný stav při bootu/resetu, 3,3V kompatibilita modulu a skutečné zapojení kontaktů.

## 6. Bezpečná synchronizace limitů horní nádrže mezi ESP32 #1 a #2
- Revize 0.24 přidává lokální autentizované AP/web rozhraní na ESP32 #2, aby šly limity a nouzový režim spravovat i při výpadku ESP32 #1.
- Do dokončení autentizovaného protokolu musí uživatel zadat stejné limity na obou jednotkách a vizuálně je porovnat. Rozdílné limity mohou znamenat, že ESP32 #1 vyhodnotí přepouštění jinak než ESP32 #2 ochranu čerpadla.
- CRC samo o sobě není autentizace. Nepřidávat řídicí UDP příkazy bez ověření identity/opravy opakování a bez bezpečného chování při výpadku komunikace.
- Místní AP používá HTTP Basic Authentication bez TLS; přihlašovací údaje se vypisují do Serial Monitoru a musí být uchovány mimo veřejně přístupná místa.

## 7. Restart transferu a trvalý journal
- Revize 0.66 ukládá PREPARED/ACTIVE/RESULT_PENDING do dvouslotového NVS journalu; PREPARED/ACTIVE se po restartu převádí na INTERRUPTED a nikdy se neobnovují.
- CRC32 chrání před náhodným poškozením, nikoli před fyzickou manipulací nebo rollbackem flash.
- Hostitelský stub ověřuje logiku a fallback, ale nenahrazuje fyzický power-cut test přesné verze ESP-IDF/Arduino core.
- Před fyzickým řízením je nutné ověřit reset v každém bodě mezi zápisem journalu a výstupem, včetně skutečného výchozího stavu ventilů a jejich napájení.
- Při výpadku NVS musí být výstupní cesta blokována; nesmí se použít RAM fallback pro povolení ventilu.


## 7. Peer výpadek během transferu z horní nádrže
- Revize 0.68 opravila pořadí diagnostiky: ztráta nebo zastarání peer dat se hlásí jako `TRANSFER_REASON_COMMUNICATION`, ne jako chyba lokálního snímače.
- PRECHECK se při výpadku odmítne; RUNNING přejde do ABORTED a vrátí `VALVES_CLOSE_ALL`.
- Zbývá fyzicky ověřit časovou odezvu skutečného W5500/UDP transportu a bezpečné elektrické uzavření ventilu. To nelze nahradit hostitelským testem.
- Peer status rámce jsou stále pouze CRC chráněné, nikoli autentizované. Nesmí se použít pro fyzické síťové řízení, dokud nebudou splněny P0 blokátory.
