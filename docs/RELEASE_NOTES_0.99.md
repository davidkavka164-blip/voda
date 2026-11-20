# HOME WATER CONTROLLER — DEV KIT 0.99

## NVS zapis kalibračních dat: povinné zpětné načtení

- `NvsSensorConfigStore::save()` po úspěšném počtu zapsaných bajtů znovu načte celý záznam a ověří validitu i checksum.
- `NvsWellGeometryStore::save()` po zápisu znovu načte geometrii a ověří checksum.
- `NvsWellCalibrationStore::save()` nyní provádí stejné read-back ověření jako `mergeAndSave()`; kontroluje checksum a počet kalibračních bodů.
- Změny jsou bajtově synchronizované mezi `firmware/common/` a oběma `firmware/ESP32_x/src/`.
- Jde o kontrolu úspěšnosti zápisu a integrity záznamu, nikoli o transakční dvouslotovou ochranu ani fyzický power-cut test.

## Ověření

- Native C++: 58/58 testovacích programů PASS.
- ASan/UBSan: 58/58 PASS.
- Python klient provisioningu: 17/17 PASS; `py_compile` PASS.
- Credential security module sync PASS.
- Arduino-ESP32 1.0.6 target build, skutečná NVS, fyzický power-cut, Flash/NVS Encryption a hardware zůstávají neověřené.
- GPIO/UART provisioning zůstává nezapojený; jednorázový Serial handoff prvotních credentials zůstává otevřený.
