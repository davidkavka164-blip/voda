# Release notes — DEV KIT 0.78

Datum: 2026-11-04

## Audit ukládání přístupových údajů
- Provedena kontrola NVS inicializace AP a administrátorských hesel obou ESP32.
- Potvrzeno, že ESP32 #1 může spustit AP s heslem pouze v RAM při nedostupném NVS nebo neúspěšném zápisu.
- Potvrzeno, že ESP32 #2 nekontroluje návratové hodnoty zápisu názvu AP a hesel a může spustit služby bez ověřeného trvalého uložení.
- Zdokumentováno vystavení AP/admin hesel v Serial Monitoru na obou jednotkách a AP hesla v dashboardu ESP32 #1.
- Přidán cílový fail-closed stav a testovací matice pro NVS begin/write/read-back/mismatch.

## Rozsah změny
- Pouze audit a dokumentace; firmware záměrně beze změny.
- Servisní GPIO nebylo přiděleno, protože přesný typ/revize desek není potvrzen.
- NVS Encryption / Flash Encryption nebyly ověřeny na cílovém hardware.

## Ověření
- Nativní sada a ASan/UBSan spuštěny při sestavení balíčku.
- Arduino-ESP32 build ani fyzické testy nebyly provedeny.
