# Release notes – REV23

## Zaměření
Audit návaznosti logických ochran čerpadel na fyzická bezpečnostní relé.

## Změny
- Přidán `PUMP_SAFETY_RELAY_INTEGRATION_AUDIT_REV23.md` s logickým kontraktem výstupů, kontrolou hlavního dokumentu 1.3 a seznamem elektrických podmínek před aktivací.
- Upřesněna předběžná mapa: ESP32 #1 GPIO25 = kandidát ochrany čerpadla studny; ESP32 #2 GPIO25 = kandidát místní ochrany čerpadla horní nádrže.
- ESP32 #1 GPIO32 je označeno jako nepřiřazené/rezervní, protože hlavní dokument 1.3 neuvádí druhé čerpadlo chráněné ESP32 #1.
- Doplněn otevřený hardwarový požadavek: potvrdit, zda 24V relé přerušuje nízkonapěťový povolovací/řídicí obvod, nebo výkonový obvod motoru, a potvrdit bezpečné zapojení COM/NO/NC.

## Záměrně beze změny
- Žádný fyzický pump safety GPIO nebyl inicializován ani ovládán.
- Nezměněny limity hladin, hystereze, logika `pumpPermit`, NVS, nouzový režim, ventily, transfery ani tlaková automatika.
- Žádný předpoklad o konkrétním zapojení kontaktů nebyl převeden do firmware.

## Ověření
- Dokumentační a statická kontrola: pump safety GPIO zůstávají bez `pinMode`/`digitalWrite`.
- Cílový Arduino build a fyzické testy nebyly provedeny.
