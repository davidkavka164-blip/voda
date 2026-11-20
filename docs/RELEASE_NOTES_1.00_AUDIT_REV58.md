# Release notes — audit REV58

- Přidán jednoduchý `PeerStatusHmacCodec`: stávající 52B status + 32B HMAC-SHA256, společný statický klíč, bez handshake/session keys.
- Přidán minimální `NvsPeerSharedKeyStore` s kontrolou délky, read-back a porovnáním klíče; výslovně plain NVS bez šifrování.
- Oba moduly jsou synchronizovány do common a obou ESP32 source trees.
- Přidány nativní testy HMAC status rámce a NVS key store; upraveny native/ASan test runners a kontrola neintegrovaného transportu.
- Produkční UDP smyčky beze změny; autentizovaný control gate zůstává `false`.
