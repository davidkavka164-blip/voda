# Release notes – REV33

## Audit obnovy komunikace a oprava stavové stránky

- Dokončena kontrola ztráty a obnovení W5500/Ethernet komunikace mezi ESP32 #1 a ESP32 #2.
- Ověřeno zachování `peerBootId` a sekvence ESP32 #2 napříč opakovanou inicializací Ethernetu.
- Ověřeno odmítání duplicitních, starých a mimo pořadí doručených peer rámců.
- Ověřeno ukončení horního transferu při nečerstvých peer datech nebo při detekci nového boot epochu ESP32 #2.
- Ověřeno, že po abortu zůstává TransferLogic v terminálním stavu a obnovení komunikace starý transfer samo neobnoví; vyžaduje se nový požadavek.
- Ověřeno, že žurnál PREPARED/ACTIVE po restartu vede do INTERRUPTED a automatický transfer se neobnoví.
- Opravena zastaralá informace na dashboardu ESP32 #1: běžné spouštění čerpadel zajišťuje tlaková automatika, ale ESP32 ovládá bezpečnostní relé, která povolují nebo přerušují napájení čerpadel.

## Testování

- Nativní testy: 65/65 PASS.
- ASan/UBSan: 65/65 PASS.
- Python testy: 17/17 PASS.
- Integrační kontroly peer boot identity, boot epoch abortu a W5500 recovery: PASS.
- Python compileall: PASS.
- ZIP integrita: PASS.
- Arduino-ESP32 cílový build a fyzické testy Ethernet recovery/relé: neprovedeny.
