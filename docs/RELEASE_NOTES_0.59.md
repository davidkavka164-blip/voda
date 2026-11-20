# Revize 0.59 — spolehlivost responder retry

## Opravené chování
- READY retry policy se nyní aktivuje při prvním platném FINISH, nikoli až po úspěšném předání prvního READY transportu.
- Při selhání prvního lokálního UDP odeslání je READY stále naplánováno k opakování.
- První úspěšný READY send nebo úspěšná retransmise přepne responder do ESTABLISHED a teprve potom zpřístupní session key.
- Opakovaný stejný FINISH vrací cacheované READY, ale neresetuje retry interval ani stav session. Pozměněný FINISH zůstává odmítnut.
- `markReadyRetryAttempted(nowMs, accepted)` započítá i neúspěšný lokální send. Při vyčerpání pokusů před prvním úspěšným READY se session ukončí a klíč se vymaže.
- `markChallengeAttempted(nowMs)` umožňuje transportu započítat neúspěšný lokální send CHALLENGE; tím se zabrání neomezenému opakování v každém průchodu loop.
- Komentář ESP32 #2 opraven: lokální logika ochrany čerpadla již integrována je, ale fyzické relé zůstává záměrně nepřipojeno.

## Ověření
- `tests/run_native_tests.sh`: 43/43 testovacích programů prošlo.
- Celá nativní sada prošla s `-fsanitize=address,undefined -fno-omit-frame-pointer`.
- Testy zahrnují ztracené první READY, neúspěšné UDP pokusy, retry exhaustion, duplicitní FINISH, změněný FINISH a wrap-safe časování.
- Společné moduly budou synchronizovány do obou Arduino `src/` stromů; následně se ověří jejich identita.

## Neověřeno / omezení
- Stále chybí W5500/UDP orchestrace, bezpečný secret provisioning a skutečný chráněný backend.
- Arduino-ESP32 build a fyzický hardware nebyly ověřeny.
- Výkonové výstupy zůstávají neaktivní.
