# HOME WATER CONTROLLER 0.49

## ESP32 Preferences/NVS backend pro persistentní anti-replay
- Přidán `NvsPeerReplayStore` jako dvouslotový žurnál v namespace `hwc_peer_rp`, klíče `slot0` a `slot1`.
- Každý checkpoint obsahuje verzi 2, identitu obou zařízení, peer boot ID, rezervovanou sekvenční hranici, generaci a CRC32.
- Při zápisu se přepisuje pouze neaktivní slot. Po úspěšném `Preferences::putBytes()` se celý žurnál znovu načte a nový checkpoint se ověří. Předchozí slot se nemaže.
- Při jednom poškozeném slotu se použije druhý platný záznam. Dva poškozené sloty, chyby čtení nebo selhání zápisu vedou k odmítnutí; nikdy se automaticky nezačíná s prázdným replay stavem.
- Návrh vychází z dokumentace ESP-IDF NVS: aktualizace zapisuje novou hodnotu před odstraněním staré a po výpadku napájení může být ztracena nová key-value položka. Dvouslotový žurnál proto drží starší checkpoint odděleně.
- `Preferences::putBytes()` ve zdrojovém kódu Arduino-ESP32 volá `nvs_set_blob()` a `nvs_commit()`.

## Ověření
- Nativní sada: 36/36 testovacích programů.
- Nový test pokrývá dva sloty, generace, chybu zápisu, poškození novějšího slotu, fallback a propojení s `PersistentPeerReplayGuard` přes restart přijímače.
- Nový NVS adaptér prošel samostatným AddressSanitizer + UndefinedBehaviorSanitizer během.

## Co stále není hotovo
- Test je hostitelská simulace Preferences, nikoli skutečný power-cut test na ESP32. Překlad v Arduino IDE 1.8.19 a fyzický test NVS zůstávají neprovedené.
- NVS replay backend zatím není aktivován v běžící firmware, protože chybí autentizované předání `persistentReplayFloor()` odesílateli a monotónní sequence allocator.
- Chybí W5500 handshake orchestrace, skutečné `esp_random()` boot ID/nonce, retry a napojení na transferový automat.
- Čerpadla a ventily zůstávají softwarově neaktivní; žádné výkonové GPIO nejsou připojeny k řízení.
