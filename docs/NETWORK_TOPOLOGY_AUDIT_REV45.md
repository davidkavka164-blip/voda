# Síťová topologie a peer UDP audit – REV45

Datum: 2026-11-13

## Uživatelem potvrzená topologie

- ESP32 #1 bude připojena k domácí Wi-Fi. Vestavěné Wi-Fi STA poskytuje hlavní webové rozhraní a přijímá stavové UDP rámce ESP32 #2.
- ESP32 #2 bude připojena do domácí LAN přes W5500 Ethernet. W5500 zajišťuje DHCP a vysílání stavových UDP rámců. Místní servisní AP/web ESP32 #2 zůstává nezávislé.
- Peer stav je jednosměrný ESP32 #2 → ESP32 #1, UDP broadcast na port 4211.

## Implementační změny REV45

- ESP32 #1: nahrazen `EthernetUDP` typem `WiFiUDP`; odstraněna inicializace W5500/SPI/DHCP z hlavní jednotky.
- ESP32 #1: UDP listener se otevře pouze při připojení STA k domácí LAN. Při ztrátě STA se socket uzavře a peer transport se označí jako nedostupný. Po obnovení Wi-Fi se listener znovu otevře.
- ESP32 #1: horní hladina je nadále použitelná pouze při připraveném peer transportu a čerstvém, validním peer rámci. Samotné připojení Wi-Fi ani otevřený UDP socket nestačí.
- ESP32 #1 fallback SoftAP není síťový bridge do domácí Ethernetové LAN. Pokud domácí Wi-Fi vypadne, horní peer data a horní transfer jsou nedostupné; lokální web/AP a místní řízení ESP32 #1 pokračují podle stávající logiky.
- ESP32 #2: W5500 Ethernet/DHCP/UDP vysílač zůstává zachován. Ztráta jeho Ethernetu neblokuje místní snímání ani místní ochranu horního čerpadla; pouze přestane být dostupný stav horní nádrže na ESP32 #1.

## Síťové předpoklady a rizika

Obě zařízení musí být ve stejné domácí IP podsíti. Router/AP musí předávat broadcast z Ethernetu do Wi-Fi STA klientů. Peer komunikace nemusí fungovat, pokud:

- ESP32 #1 používá guest Wi-Fi nebo oddělenou VLAN;
- je zapnutá AP/client isolation nebo blokování komunikace mezi kabelovými a bezdrátovými klienty;
- router/AP filtruje broadcast UDP;
- ESP32 #1 zůstane pouze ve fallback SoftAP režimu bez STA připojení.

Doporučené fyzické ověření: připojit ESP32 #1 k běžné domácí Wi-Fi (nikoliv guest síti), ESP32 #2 k LAN portu routeru/switche, ověřit IP adresy a masku ve stejné podsíti a sledovat přijetí sekvenčních peer rámců. Poté postupně odpojit Ethernet ESP32 #2, vypnout Wi-Fi ESP32 #1, obnovit obě linky a ověřit, že horní transfer zůstane zablokován až do přijetí nového čerstvého rámce.

## Bezpečnostní chování

- Ztráta STA na ESP32 #1 uzavře peer UDP listener; starý stav se nesmí používat pro horní transfer.
- Po reconnectu se starý cached frame nepovažuje za čerstvý; musí přijít nový peer status.
- Ztráta Ethernetu ESP32 #2 vede ke stárnutí peer rámce a zablokování horního transferu na ESP32 #1.
- Peer UDP status je stále broadcast v místní LAN; tato revize nepřidává aplikační autentizaci ani šifrování. Síť musí být důvěryhodná a izolovaná od nedůvěryhodných klientů.
- Žádný webový server není vystaven přes W5500; vzdálený přístup zůstává přes VPN routeru, nikoliv přímým port-forwardingem HTTP.

## Testy a omezení

- Přidán statický test `check_mixed_wifi_ethernet_peer_transport.sh`.
- Aktualizovány `check_network_recovery_safety.sh` a `check_peer_ethernet_recovery.sh`.
- Hostitelské nativní testy a statické kontroly ověřují volbu rozhraní a fail-closed gate při ztrátě STA.
- Neověřeno: Arduino-ESP32 target build, skutečný broadcast průchod konkrétním routerem/AP, Wi-Fi/Ethernet interoperability na fyzických deskách, DHCP a ztráta/obnova linku v reálné instalaci.

REV45 implementuje uživatelem potvrzenou topologii ve zdrojovém kódu, ale před připojením řízení do ostrého provozu je nutné provést fyzický test broadcastu a cílovou kompilaci.
