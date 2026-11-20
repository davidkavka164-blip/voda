# Release notes – REV19

- Doplněn samostatný servisní/testovací režim ručního ovládání ventilů podle Master 1.3 §40.
- Nouzový režim zůstává oddělený, vyžaduje relevantní poruchu čidla a fyzické potvrzení hladin.
- Servisní režim vyžaduje čerstvá a platná relevantní čidla.
- Oba režimy používají stejné relé GPIO16/17 a zachovávají dosavadní ochrany.
- Nouzové fyzické potvrzení může nahradit pouze nedostupné měření studny; dolní nádrž a horní peer data nelze obejít.
- Automatický transfer je při ručním režimu blokován; čerpadla zůstávají mimo řízení ESP32.
- Nebyly změněny hladinové limity, rezervy, hystereze ani ochrana čerpadel.
