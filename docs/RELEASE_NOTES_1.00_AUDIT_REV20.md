# Release notes – REV20

- Nouzový ruční režim lze aktivovat bez ohledu na to, zda je porucha čidla přítomna.
- Nouzové povely pro oba směry obcházejí procesní kontroly měření dolní nádrže, měření studny, komunikace s ESP32 #2, hladinových limitů a horní hystereze.
- Servisní režim zůstává beze změny a vyžaduje funkční relevantní čidla.
- Zachovány jsou hardwarové a stavové interlocky: reléový driver musí být připraven, automatický transfer musí být ukončen, restartový žurnál musí dovolovat ruční převzetí, nikdy nejsou sepnutá obě relé a změna směru vyžaduje STOP.
- Potvrzení obsluhy a fyzické ověření situace před otevřením zůstává povinné.
- Reléový modul: 5 V napájení; ESP32 výstupy GPIO16/17, aktivní HIGH.
- Žádné automatické transferové prahy, rezervy, hystereze ani ochrany čerpadel nebyly změněny.
