# Ruční ovládání ventilů – implementace REV20

## Změna proti REV19

Na výslovný požadavek obsluhy nouzový ruční režim již není podmíněn poruchou čidla. Lze jej aktivovat i při plně funkčních čidlech. Po potvrzení obsluhy lze vydat povel pro kterýkoli ventil bez ohledu na:

- platnost nebo dostupnost měření dolní nádrže,
- platnost nebo dostupnost měření studny,
- dostupnost nebo čerstvost dat horní nádrže z ESP32 #2,
- běžné hladinové limity, rezervy a hysterezi horního ventilu.

Nouzový režim je výslovný provozní override. Nejde o automatický transfer a po restartu se neobnovuje.

## Co zůstává zachováno

- Výslovné potvrzení obsluhy při aktivaci nouzového režimu.
- Připravený reléový výstup ESP32 #1.
- Automatický transfer musí být ukončen; nouzový režim nepřebíjí souběžně běžící automatickou sekvenci.
- Restartový žurnál musí být ve stavu umožňujícím bezpečné ruční převzetí.
- Současně může být sepnut pouze jeden ventilový výstup.
- Před změnou směru se musí použít STOP/CLOSE_ALL.
- STOP, ukončení režimu, restart a selhání reléového driveru vedou k povelu obě relé OFF.
- ESP32 #2 nepřebírá ovládání relé ventilů; čerpadla nejsou řízena těmito výstupy.
- Stav zobrazený v UI je stav povelu, nikoli potvrzená fyzická poloha ventilu.

Servisní režim se nemění: nadále vyžaduje platná a aktuální relevantní čidla a zachovává běžné bezpečnostní podmínky.

## Uživatelské rozhraní

UI výslovně upozorňuje, že nouzový režim obchází měření hladin i komunikaci s ESP32 #2 a vyžaduje fyzické ověření situace obsluhou před otevřením ventilu.

## Hardware

Reléový modul je napájen 5 V. Ovládací signály jsou GPIO16 a GPIO17, aktivní HIGH. Ventily jsou bez napájení zavřené; pro tento režim je použito zapojení kontaktů COM–NO. Kompatibilita vstupů relé modulu s logikou 3,3 V ESP32 a fyzické zapojení musí být ověřeny na konkrétním modulu.

## Neověřené

Arduino cílový build ani fyzická zkouška ESP32, relé, ventilů a vodní soustavy nejsou součástí této revize.
