# HOME WATER CONTROLLER — REV34

## Síťová obnova a dostupnost webu

- Proveden audit Wi-Fi STA, fallback AP, lokálního AP ESP32 #2 a W5500 peer komunikace.
- Upřesněna dokumentace: W5500 je v této verzi pouze UDP peer transport, nikoli alternativa webového serveru.
- ESP32 #1 opakuje připojení k uložené Wi-Fi každých 15 s; při pozdějším výpadku spustí fallback AP po 30 s. AP zůstává aktivní i po obnovení STA do restartu, aby lokální servisní přístup nezmizel.
- ESP32 #2 zachovává vlastní lokální AP a místní ochranu horního čerpadla nezávisle na peer Ethernetu.
- Omezena délka DHCP retry z hlavní smyčky z 5 s na 1 s (response timeout 250 ms). Startovní DHCP inicializace si ponechává 5 s.
- Přidán `tests/check_network_recovery_safety.sh` do hlavní testovací sady.

## Ověření

- Hostitelské nativní testy a statické integrační kontroly jsou součástí balíčku.
- Skutečné Wi-Fi/AP/DHCP/W5500/UDP chování, fyzická obnova kabelu a cílový Arduino-ESP32 build nebyly v tomto prostředí provedeny.
