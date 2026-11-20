# Release notes — REV61

- Master aktualizován na 1.5 podle uživatelem schválené architektury dvou nezávislých ESP32 bez aplikační komunikace.
- ESP32 #1 je jediným místem ovládání transferů studna → dolní a horní → dolní; požadované množství se zadává v litrech přes jeho web.
- ESP32 #2 samostatně chrání horní čerpadlo, měří hladinu a teplotu a bude poskytovat objem horní nádrže z ručně kalibrované křivky.
- Výslovně potvrzen požadavek na detekci nulového i přerušeného přítoku horní → dolní podle přírůstku dolní nádrže.
- Časové/objemové prahy hlídání přítoku zůstávají neurčené do změření reálného snímače a průtoku.
- REV61 je dokumentační aktualizace. Provozní firmware zůstává zatím beze změny; peer závislosti tedy ještě nejsou odstraněny.
- Cílový Arduino build ani hardware testy REV61 nebyly provedeny.
