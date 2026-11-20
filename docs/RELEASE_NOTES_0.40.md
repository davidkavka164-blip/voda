# Revize 0.40 — fail-closed kontrakt pro úložiště peer tajemství

- Přidán `PeerSecretVault.*` a rozhraní `PeerSecretBackend`. Tajemství má přesně 32 bajtů, musí být nenulové a je svázáno s uspořádanou dvojicí lokální/peer ID.
- Načtení, provisioning i mazání jsou odmítnuty, pokud backend výslovně nepotvrdí dostupnost důvěrného úložiště. Provisioning a mazání navíc vyžadují explicitní potvrzení fyzického provisioning postupu.
- Nechráněné NVS/Preferences bez konfigurace ESP-IDF NVS encryption se nesmí implementovat jako backend vracející `true`. Tento modul neposkytuje šifrování, distribuci klíče ani bezpečnostní garanci pro flash; je to pouze bezpečnostní kontrakt a testovatelná politika.
- Přidán test pro odmítnutí nechráněného backendu, chybějícího fyzického potvrzení, neplatné/stejné ID, vazbu klíče na dvojici, bezpečné nulování výstupu a mazání.
- Nativní testovací programy: 26/26 prošly. Celá sada prošla AddressSanitizer + UndefinedBehaviorSanitizer.
- Společné moduly synchronizovány do obou sketch `src` složek. Arduino-ESP32 build, skutečný NVS encryption, provisioning na zařízení, síť a hardware nebyly ověřeny. Žádné výkonové výstupy nejsou aktivní.
