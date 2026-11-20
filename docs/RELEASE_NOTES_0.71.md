# HOME WATER CONTROLLER — DEV KIT 0.71

## Obnova peer transportu po ztrátě DHCP adresy

- ESP32 #2 nyní po `Ethernet.maintain()` ověřuje `Ethernet.localIP()`. Pokud adresa zmizí, zavře UDP socket, označí peer Ethernet jako nedostupný a spustí časovaný retry režim.
- ESP32 #1 při zjištění ztráty DHCP adresy také zavře UDP socket a připraví další inicializaci.
- Přidán `tests/check_peer_ethernet_recovery.sh`, který kontroluje fail/retry cestu obou endpointů; je součástí `run_native_tests.sh`.
- Boot ID a sekvence ESP32 #2 zůstávají stabilní přes síťový reconnect.

## Ověření

- Standardní nativní sada: 45/45.
- AddressSanitizer + UndefinedBehaviorSanitizer: 45/45.
- Statická kontrola boot identity: PASS.
- Statická kontrola DHCP recovery obou endpointů: PASS.
- Fyzické odpojení Ethernet kabelu, W5500, DHCP lease expiry a opětovné navázání linky nebylo testováno. Kontrola `localIP()==0` nepředstavuje detekci každého fyzického výpadku kabelu.
- Arduino-ESP32 build nebyl proveden; fyzické výstupy zůstávají zakázané.
