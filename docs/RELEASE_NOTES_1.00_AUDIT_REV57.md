# Release notes — audit REV57

- Přidán `PEER_AUTH_SIMPLIFICATION_AUDIT_REV57.md`.
- Rozhodnutí: pro jednosměrný peer status použít jednoduchý HMAC-SHA256 nad stávajícím 52B rámcem; bez handshake/session orchestrace a bez secure elementu.
- Zdokumentováno omezení plain NVS a přesný rozsah ochrany sequence/bootId včetně ztráty replay historie po restartu přijímače.
- Firmware, síťová topologie, GPIO, NVS layout, partition table, eFuse a bezpečnostní brána se nemění.
