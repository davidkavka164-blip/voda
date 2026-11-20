# ESP32 #2 — horní nádrž

Tato revize čte místní snímač hladiny a vyhodnocuje logickou ochranu čerpadla nezávisle na ESP32 #1. Ochrana načítá meze z vlastního NVS namespace `hwc_upperlimits`. Protože NVS každé ESP32 je fyzicky samostatné, uložení mezí na ESP32 #1 je automaticky nedostane do této jednotky. Dokud není platná místní konfigurace bezpečně zavedena, povolení čerpadla zůstává blokováno.

Po startu i po neplatném/stárnoucím měření se vyžaduje dosažení nastavené obnovovací hladiny. Při dosažení kritické hladiny se blokace aktivuje a uvolní se až při obnovovací hladině. Ve starších revizích zůstával výstupní GPIO/relé vypnutý; od REV28 je fyzický výstup připojen k bezpečnostnímu relé čerpadla na GPIO25.

Skutečný překlad Arduino-ESP32 a test na hardwaru nejsou provedeny.


## REV67 — objemová kalibrace horní nádrže

ESP32 #2 nyní obsahuje samostatnou objemovou kalibraci výška → absolutní litry. První bod musí být ručně ověřená absolutní kotva. Další body lze přidat nebo upravit; mezi body se objem interpoluje a mimo rozsah se neextrapoluje. Před transferem uživatel uloží výchozí stav, po transferu zadá skutečné litry z ESP32 #1. Nový bod se uloží pouze po fyzickém potvrzení a při zachování monotónní křivky. Kalibrace slouží pouze k zobrazení objemu a neovlivňuje ochranu čerpadla. Záznam je uložen v NVS `hwc_up_vol` s CRC a zpětným ověřením.

Webové rozhraní ESP32 #2 je nadále dostupné pouze přes servisní AP. W5500 získává DHCP, ale HTTP server přes Ethernet není implementován.

## OTA ESP32 #2 — revize 0.26
Místní autentizované webové rozhraní nyní obsahuje upload `.bin` přes `Update`. Upload kontroluje autorizaci, neúplný zápis, přerušení a dokončení `Update.end(true)`. Po úspěšném dokončení odešle potvrzovací stránku a restartuje jednotku. Funkce není ověřena na reálné OTA partition ani zařízení; před použitím musí být ověřeno, že tabulka partition podporuje OTA a že soubor `.bin` odpovídá ESP32 #2.

Revize 0.27: upload odmítá soubory bez `.bin` přípony a nepokračuje, pokud je logicky povolené čerpadlo nebo aktivní nouzový režim. Přípona souboru ale sama neověřuje cílové zařízení ani správnost partition.

Revize 0.28: kritická blokace horního čerpadla se ukládá do odděleného NVS záznamu s CRC. Pokud záznam chybí, nouzové povolení zůstává zakázáno, dokud platné měření nedosáhne obnovovací hladiny; poškozený záznam se automaticky nepřepisuje.


REV22: Lokální logické vyhodnocení ochrany horního čerpadla pokračuje nezávisle na peer komunikaci s ESP32 #1. Při vypnutí nebo poruše samotného ESP32 #2 ochrana neběží.

REV28: Fyzické bezpečnostní relé horního čerpadla na GPIO25 je povoleno. Používá stejné zapojení Drátek 24 V / 30 A HIGH–LOW jako samostatná studna (24 V DC, jumper HIGH, aktivní HIGH z GPIO). Výstup je řízen místní ochranou a místním časem posledního měření; peer komunikace s ESP32 #1 není podmínkou ochrany.

REV29: Audit potvrdil, že horní čerpadlo používá pouze místní snímač, konfiguraci, kritický latch a lokální `pumpPermit`; peer komunikace s ESP32 #1 není podmínkou jeho ochrany. Nouzový režim čerpadla je oddělen od nouzového ovládání ventilů. Viz `docs/RELEASE_NOTES_1.00_AUDIT_REV29.md`.


REV30: Web ESP32 #2 nyní zobrazuje stav místní ochrany i povel bezpečnostnímu relé. Nouzové povolení při poruše horního čidla zůstává samostatné, vyžaduje fyzické potvrzení a po restartu se neobnovuje.

REV91: Detailní návrh bezpečného společného HTTP transportu pro servisní AP a W5500 je v `docs/HTTP_TRANSPORT_ADAPTER_DESIGN_REV91.md`. Produkční HTTP zůstává zatím na servisním AP; žádná routa ani ochranná logika nebyla změněna.
