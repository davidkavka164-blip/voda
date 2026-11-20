# NVS write-integrity audit — DEV KIT 0.99

## Scope

Static review of Preferences-backed stores and host tests. No ESP32 board, flash, eFuse, partition table, or physical power-cut operation was used.

## Changes in 0.99

Three calibration/configuration stores previously returned success after checking only the `putBytes()` byte count:

- `NvsSensorConfigStore::save()`
- `NvsWellGeometryStore::save()`
- `NvsWellCalibrationStore::save()`

They now read the record back through their normal validated `load()` path and verify the persisted checksum; calibration additionally checks point count. The source copies in `common`, ESP32 #1, and ESP32 #2 are synchronized.

## Existing protections observed

- Sensor config, well geometry, well calibration, well protection, upper-tank limits/protection, transfer history, and transfer journal records validate size/format and/or checksum before accepting data.
- Several stores refuse to overwrite an already-present record that fails validation.
- Transfer journal and peer replay state use multi-record/checkpoint strategies with recovery logic.
- Credentials use a two-slot write/read-back/selector commit and fail closed on incomplete/corrupt state.
- Peer shared-secret storage has an explicit confidentiality contract; plaintext Preferences must not be accepted as a secure backend.

## Remaining limitations / risks

- Read-back after a single-key write detects a failed or malformed persisted value but does not preserve the previous value if the underlying NVS entry itself is lost or corrupted.
- Not every Preferences-backed store is transactional or double-buffered. A full per-store power-cut campaign remains necessary before hardware activation.
- CRC/checksum detects accidental corruption; it is not authentication and does not prevent deliberate modification by someone with flash access.
- AP/admin credentials remain in ordinary Preferences/NVS unless platform encryption is actually configured and verified.
- Arduino-ESP32 core version and partition layout are not confirmed on target; no claim of NVS Encryption or Flash Encryption is made.

## Safety decision

Keep peer command path default-deny and all physical outputs inactive. Do not enable eFuses, Flash Encryption, NVS Encryption, or change partition layout as part of this host-only revision. Hardware validation must use a recoverable test board and a documented backup/provisioning procedure.
