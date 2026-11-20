# Release notes – REV22

- Nouzový režim studniční trasy nově zachovává stávající ochranu čerpadla proti chodu nasucho, pokud je měření studny platné a čerstvé: otevření je povoleno pouze při `pumpProtectionState.pumpPermit == true`.
- Pokud je měření studny neplatné, nouzový ventilový povel může být povolen po fyzické kontrole a potvrzení obsluhy; ochranu založenou na tomto čidle nelze v takovém stavu vyhodnotit.
- Ochrana horního čerpadla se nadále vyhodnocuje lokálně na ESP32 #2 nezávisle na peer komunikaci s ESP32 #1. Při výpadku samotného ESP32 #2 ji nelze zaručit.
- `pumpProtectionReady` zůstává diagnostický údaj a není novou blokací automatického transferu ani nouzového ventilového režimu.
- Nouzový režim nadále obchází dolní hladinové měření, běžné transferové prahy/hysterezi a peer komunikační podmínku pro horní ventil; zachovává fyzickou kontrolu obsluhy a ON/STOP princip.
- Bez změny limitů, rezerv, hysterezí, mapy ventilových GPIO16/17 a napájení 5V reléového modulu.
- Fyzické pump safety relay výstupy zůstávají v této revizi neaktivované; logické vyhodnocení ochrany není důkazem fyzického odpojení čerpadla.
