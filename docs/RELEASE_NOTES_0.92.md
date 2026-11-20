# HOME WATER CONTROLLER DEV KIT 0.92

Datum: 2026-11-04

## Hostitelský klient lokálního provisioningu

- Přidán `tools/credential_provisioning_client.py` pro sestavení SET_CREDENTIALS v1 rámce a kontrolu 9B výsledkové odpovědi.
- Hesla se zadávají pomocí `getpass`; nejsou argumenty CLI a nejsou vypisována ani logována.
- Výchozí režim je pouze náhled, bez odesílání.
- Reálné odesílání vyžaduje `--send`, port a explicitní `--firmware-integrated`; aktuální firmware tuto integraci nemá a nesmí se s ním odesílat.
- PySerial 3.5 je volitelná závislost pouze pro pozdější COM/UART použití.
- DTR/RTS jsou před otevřením portu nastaveny na neaktivní úroveň, ale auto-reset konkrétní desky není ověřen.
- Přidáno 15 Python unit testů pro CRC, frame layout, cross-language golden vector, validaci vstupů, response parsing, fake serial exchange, ochranu náhledu a blokaci nepovoleného odesílání.

## Výsledky

- `./tests/run_native_tests.sh`: 50/50 PASS.
- `sh tests/.run_native_tests_asan.sh`: 50/50 PASS.
- `python3 -m unittest discover -s tests/python -v`: 15/15 PASS.
- `python3 -m py_compile tools/credential_provisioning_client.py tests/python/test_credential_provisioning_client.py`: PASS.
- `tests/check_peer_boot_identity.sh`: PASS.
- `tests/check_peer_ethernet_recovery.sh`: PASS.

## Omezení

- Klient není ověřen na fyzickém COM/UART a aktuální firmware nemá device-side Serial/GPIO adapter.
- `--firmware-integrated` se nesmí použít u revize 0.92.
- Protokol přenáší credentials v plaintextu; pouze přímý lokální kabel, žádný bridge.
- Fyzický servisní vstup, AP boot gate, NVS/Flash Encryption, Arduino build a hardware zůstávají otevřené.
- Výpisy hesel v Serial/HTML zůstávají do ověřené migrace/obnovy; fyzické výstupy jsou nadále neaktivní.
