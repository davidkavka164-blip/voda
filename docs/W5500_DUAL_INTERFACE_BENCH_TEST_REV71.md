# W5500 + servisní AP – izolovaný diagnostický test (REV71)

## Účel

Nový sketch `tools/W5500_DUAL_INTERFACE_DIAGNOSTIC/W5500_DUAL_INTERFACE_DIAGNOSTIC.ino` ověřuje souběh:

- Wi-Fi servisního AP obsluhovaného `WebServer`;
- W5500 Ethernetu obsluhovaného `EthernetServer` z již používané knihovny `Ethernet.h`.

Je to pouze čtecí diagnostická stránka. Neobsahuje řízení čerpadel/ventilů, NVS, přihlášení, POST, OTA ani zápis konfigurace. Nesmí se zaměnit za produkční firmware.

## Proč tento krok

Stávající firmware ESP32 #2 používá `WebServer` na Wi-Fi AP a `Ethernet.h` pro W5500 DHCP. Tyto dvě knihovny neposkytují automaticky společný serverový socket. `EthernetServer` je proto nejmenší izolovaný test bez přidávání další ethernetové HTTP knihovny. Pokud test projde na skutečné desce, bude možné navrhnout sdílenou vrstvu rout, ale nikoli automaticky přenést všechny produkční routy.

## Zapojení a bezpečnost

Používá stávající mapu W5500 ESP32 #2: SCK GPIO18, MISO GPIO19, MOSI GPIO23, CS GPIO27, RST GPIO26. Testovací AP má dočasné heslo uvedené v souboru sketch; před použitím je nutné jej změnit. W5500 připojit pouze do důvěryhodné lokální sítě. Sketch není určen pro trvalý provoz.

## Postup ověření

1. V Arduino IDE 1.8.19 vybrat stejnou ESP32 desku jako pro ESP32 #2.
2. Zaznamenat přesnou verzi `esp32 by Espressif Systems` v Boards Manageru a verzi knihovny Ethernet.
3. Otevřít samostatný sketch z adresáře `tools/W5500_DUAL_INTERFACE_DIAGNOSTIC`.
4. Změnit dočasné heslo AP, připojit W5500 do důvěryhodné sítě a nahrát sketch.
5. V Serial Monitoru při 115200 Bd zaznamenat AP IP a W5500 DHCP IP.
6. Otevřít `http://192.168.4.1/` přes AP a současně `http://<W5500-IP>/` přes LAN. Ověřit opakované obnovení obou stránek.
7. Odpojit Ethernet kabel: AP musí zůstat dostupné. Znovu připojit kabel a ověřit DHCP retry.
8. Zapsat výsledky, verze knihoven, desku a případné chyby překladu.

## Omezení a rozhodovací brána

- V tomto vývojovém prostředí nebyl k dispozici Arduino cílový překladač ani fyzický W5500; sketch zde nebyl zkompilován ani spuštěn.
- Úspěch tohoto testu prokáže jen současné doručení jednoduchého GET `/` přes oba transporty.
- Neprokáže bezpečné přenesení Basic Auth, Origin/Host guard, POST, OTA upload, limity klientů ani odolnost produkční smyčky.
- Produkční `ESP32_2.ino` zůstává beze změny: web je nadále pouze přes servisní AP. Do implementace LAN webu se nepokračuje, dokud nebude tento test fyzicky ověřen a zjištěna přesná verze core/knihovny.


## Dodatečné zpevnění parseru (REV72)

Příjem Ethernet HTTP požadavku používá neblokující stavový parser s pevnými poli: request line max. 95 znaků, nejvýše 15 hlaviček po max. 127 znacích a společný časový limit 500 ms. V jednom průchodu smyčkou zpracuje nejvýše 64 přijatých bajtů, takže Wi-Fi AP může dál obsluhovat klienty. Parser nepoužívá `readStringUntil()` ani dynamicky rostoucí `String` pro příchozí request/headers. Přijímá pouze `GET / HTTP/1.0` a `GET / HTTP/1.1`; ostatní nebo neúplné požadavky uzavře odpovědí 400. Tím se omezuje blokování hlavní smyčky a alokace paměti při diagnostickém testu. Toto zpevnění stále není bezpečnostní audit produkčního HTTP serveru.
