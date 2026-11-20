# HOME WATER CONTROLLER DEV KIT 0.87

Datum: 2026-11-04

## Lokální servisní autorizace — přenositelná politika

Přidán `LocalServiceAuthorization.h`, přenositelná politika jednorázové fyzické autorizace servisního režimu:

- nepřetržité aktivní držení fyzického vstupu po dobu 3 s;
- po splnění vznikne jednorázové autorizační okno 60 s;
- autorizaci lze spotřebovat pouze jednou;
- další autorizace vyžaduje uvolnění vstupu a nové podržení;
- časování používá unsigned rozdíl a je bezpečné přes přetečení `millis()`.

Modul je synchronizován v `firmware/common` a ve zdrojových stromech ESP32 #1 a #2.

## Bezpečnostní omezení

Toto je pouze přenositelná politika, nikoli hotové fyzické tlačítko ani provisioning. Žádný GPIO nebyl přidělen. Deskový adapter musí být jediným volajícím `samplePhysicalInput()` a nesmí předávat hodnotu z HTTP, UDP, Wi-Fi, sériového příkazu ani webového checkboxu. Dokud není ověřen fyzický vstup na konkrétních deskách, modul není připojen do `.ino` a neautorizuje žádnou reálnou operaci.

## Testy

- Standardní nativní testy: 48/48.
- ASan/UBSan nativní testy: 48/48.
- Peer boot identity: PASS.
- W5500 link recovery: PASS.
- Nový test pokrývá krátký stisk, zákmit, podržení, jednorázové použití, nutnost uvolnění, vypršení autorizačního okna a přetečení časovače.
- ZIP integrita ověřena po vytvoření archivu.

Arduino build, fyzický vstup, provisioning, NVS/Flash Encryption a hardware testy nebyly provedeny. Peer command path zůstává default-deny; fyzické výstupy zůstávají neaktivní.
