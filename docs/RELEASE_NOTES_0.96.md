# HOME WATER CONTROLLER DEV KIT 0.96

Datum: 4. 11. 2026

## Cíl revize

Opravit inicializaci entropy source pro nově generovaná výchozí AP a administrátorská hesla a zvýšit jejich výchozí sílu.

## Změny

- Audit 0.95 zjistil, že `esp_random()` bylo voláno před inicializací Wi-Fi driveru; pro ESP-IDF 3.3.x bez aktivního Wi-Fi/Bluetooth nelze tato volání považovat za garantovaně true-random.
- Přidán společný `CredentialRandom.h`: bezpečné hex formátování celých 32bitových slov do pevného bufferu, čtyři slova = 32 hex znaků (128 bitů).
- Oba sketchy před generováním chybějícího hesla provedou `WiFi.persistent(false)` a vyžadují úspěšné `WiFi.mode(WIFI_STA)`. Pokud Wi-Fi driver nelze spustit, nové tajemství se nevytvoří.
- Nově vytvářená AP a admin hesla mají 32 hex znaků. SSID zůstává netajné a odvozené z MAC.
- Již existující hesla se automaticky nerotují; bezpečný provisioning/obnova ještě nejsou připojeny do `.ino`.
- Odstraněna nadbytečná vnitřní forward deklarace `CredentialLegacyMigrator` ve třídě `CredentialStore`; globální forward declaration a friend zůstává.
- Přidán `test_credential_random.cpp` a statická kontrola `check_credential_entropy_integration.sh`; nový modul je synchronizován ve common a obou sketch src.

## Testy

- `./tests/run_native_tests.sh`: PASS, 56/56 native test programs.
- `sh tests/.run_native_tests_asan.sh`: PASS, 56/56 native test programs.
- Peer boot identity: PASS.
- W5500 recovery: PASS.
- Credential module synchronization: PASS.
- Credential startup/migration integration: PASS.
- Credential entropy integration: PASS.
- Python client: 17/17 PASS.

## Bezpečnostní hranice a omezení

- ESP-IDF 3.3.x dokumentuje true-random výstupy při aktivním Wi-Fi/Bluetooth; `WiFi.mode(WIFI_STA)` API pro Arduino-ESP32 1.0.6 bylo ověřeno proti upstream zdroji, ale aktuální sketch nebyl přeložen skutečným Arduino toolchainem.
- Kvalita RNG na cílových deskách nebyla změřena; host test neprokazuje fyzickou entropy.
- Již existující starší hesla se nemění. Jednorázový Serial handoff nově generovaných hesel zůstává dočasný.
- `hwcCred` zůstává plaintext NVS; NVS/Flash Encryption, skutečná NVS, power-cut a fyzický hardware nejsou ověřeny.
- Provisioning UART/GPIO zůstává nezapojen. Peer command path zůstává default-deny; fyzické výstupy jsou neaktivní.
