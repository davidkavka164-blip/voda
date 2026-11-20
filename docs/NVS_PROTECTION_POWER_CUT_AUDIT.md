# NVS protection-store power-cut audit

**Baseline:** HOME_WATER_CONTROLLER_DEV_KIT_0.99  
**Working status:** two-slot implementation and host-side fault-injection tests added; target and physical validation remain outstanding.

## Implemented in this working copy

- `NvsWellProtectionStore` and `NvsUpperTankProtectionStore` now persist to independent `slot0` and `slot1` keys.
- Each slot has a format marker, format version, payload length, monotonically increasing generation, payload and CRC.
- Writes target the inactive/older slot, require an exact-length `putBytes`, then perform a full read-back and validation.
- The previously selected slot is not intentionally overwritten during the same update.
- `hasRecord()` recognizes either new slot key or the legacy `state` key. A malformed/present record is not mistaken for a fresh installation.
- When no new slot exists, `load()` validates the legacy `state` record and migrates it into `slot0`; the legacy key is retained. The migration is accepted only after slot read-back validation.
- Well-protection migration preserves `criticalLatch` and `safetyStateValid` exactly, including `safetyStateValid=false`.
- Upper-tank migration accepts only the legacy valid state, matching the original store's `stateValid == 1` requirement.
- If either new slot exists but is malformed, or two valid slots have an ambiguous equal generation, loading fails closed. The store does not fall back to an older slot when another slot is corrupt.
- Both stores are byte-identical in `common`, `ESP32_1` and `ESP32_2`.

## Host-side tests added/updated

- Multi-key Preferences model is isolated to the two protection-store test binaries.
- Tests cover normal two-slot updates, corruption rejection, partial write to the inactive slot, preservation of the previous slot bytes, legacy migration and malformed legacy rejection (upper tank).
- The test backend is a deterministic host simulation. It is not an ESP32 NVS emulator and does not model flash erase/program internals or real power removal.

## Important limitations / open validation

1. Strict corruption policy intentionally sacrifices availability: if a partially written inactive slot is detected, the store refuses to load even though the other slot's bytes remain intact. Recovery requires an explicit, separately designed procedure; no automatic fallback is performed.
2. The current host backend's partial-write behavior models a shortened key value. It does not prove how Arduino Preferences / ESP32 NVS commits or recovers a blob under actual power loss.
3. Generation wrap is practically remote but needs an explicit policy before long-term production deployment. Current code skips generation zero; exhaustive wrap behavior is not physically tested.
4. Arduino-ESP32 1.0.6 target compilation has not been run in this environment.
5. No physical ESP32 NVS test, power-cut test, Flash/NVS encryption test, or connected-hardware test has been performed.
6. GPIO/UART credential provisioning and all physical output paths remain outside this change; no pump/valve output was activated.

## Current verification (host-side working copy, 2026-11-06)

- `tests/run_native_tests.sh`: **58/58 native test programs PASS**; all integration checks PASS, including protection-store mirror check.
- `sh tests/.run_native_tests_asan.sh`: **58/58 native test programs PASS** under AddressSanitizer and UndefinedBehaviorSanitizer; all integration checks PASS.
- `python3 -m unittest discover -s tests/python -p 'test_*.py'`: **17/17 PASS**.
- `python3 -m compileall -q tools tests/python`: PASS.
- Working ZIP integrity: pending final packaging check.
- Arduino-ESP32 1.0.6 target compilation: unavailable (neither `arduino-cli` nor PlatformIO is installed in this environment).

## Release status

This is an audit working copy based on 0.99, not a released version. Version 0.99 remains the last released/verified baseline. Host tests pass, but target compilation and physical NVS/power-cut/hardware validation remain outstanding; do not deploy this working copy to live pump control until those checks and a documented recovery procedure are completed.

## Follow-up review — 2026-11-06

A second source review of the 1.00 draft found that the well-slot CRC originally hashed the raw `WellProtectionRecord` object representation. That representation can contain compiler padding bytes, so it is not a stable serialization contract. The well-slot CRC now hashes each semantic field explicitly, and read-back payload equality is field-by-field rather than `memcmp` over the struct representation. The common and both sketch copies were synchronized.

Additional host tests now exercise interruption while reusing a slot that already contains an older valid generation. They verify that the newest slot's bytes remain unchanged while the strict loader rejects the resulting ambiguous/corrupt pair. This confirms the intended fail-closed policy in the host model; it does not establish real ESP32 NVS power-loss behavior.

Follow-up verification:

- `tests/run_native_tests.sh`: **58/58 PASS**, including integration checks.
- `sh tests/.run_native_tests_asan.sh`: **58/58 PASS**, including integration checks.
- Python unittest discovery: **17/17 PASS**.
- `python3 -m compileall -q tools tests/python`: PASS.
- `arduino-cli`, PlatformIO and `pio` are not installed in this environment; target compilation remains unavailable.
- The revised ZIP archive integrity check passed.

Remaining release blockers are unchanged: compile against the user's Arduino-ESP32 1.0.6 environment, validate Preferences/NVS behavior on real ESP32 hardware, perform controlled physical power interruption, and document/verify a deliberate recovery procedure for a corrupt slot. Do not deploy this draft to live pump control.


## Follow-up review — 2026-11-07

A further source review tightened generation ordering and serialization:

- Both stores now accept two valid slots only when their generations are exactly consecutive. The writer always increments by one and refuses generation wrap to zero; a non-adjacent pair is treated as ambiguous and fails closed. This avoids relying on signed-difference ordering for arbitrary generation values.
- Upper-tank slot CRC now hashes semantic payload fields explicitly rather than the raw `LegacyRecord` object representation. Read-back equality is also field-by-field, so compiler padding is not part of the storage contract.
- The rule is mirrored byte-for-byte across `common`, ESP32 #1 and ESP32 #2.

### Recovery policy (operator/service procedure; no automatic repair)

If either protection store reports a corrupt or ambiguous slot:

1. Keep the affected pump/transfer permission blocked. Do not clear NVS, remove `state`, or delete either slot as a first response.
2. Isolate the relevant pump/valve output using the installation's physical electrical isolation procedure before service work. Do not rely only on a web status display.
3. Record the serial diagnostic and preserve a read-only dump of the namespace/keys before changing anything. The current firmware does not provide a supported export/repair UI; do not improvise one by writing guessed bytes.
4. Verify the actual water level independently and inspect sensor wiring and supply. A valid sensor reading alone does not prove that the saved critical latch may be cleared.
5. Restore only from a known-good backup or a separately reviewed service utility that validates the complete record and requires explicit physical confirmation. If no trustworthy backup exists, treat the latch/configuration as unknown and keep the pump blocked until the protection is recommissioned and tested.
6. After service, verify cold boot, latch persistence, hysteresis/recovery behavior, and the physical output interlock before returning to operation.

This is a conservative service procedure, not an implemented automatic recovery feature. A dedicated diagnostic/export/repair mechanism remains a future requirement and must not silently reset the safety latch.

Additional release blocker: no physical ESP32 power-cut test or Arduino-ESP32 1.0.6 target build has been performed. Host simulation results must not be represented as proof of flash/NVS atomicity.

## Follow-up review — 2026-11-07: configuration and transfer journal stores

The audit was extended beyond the two persistent pump-protection latch stores.

- `NvsWellGeometryStore` and `NvsWellCalibrationStore` are single-key blob stores with CRC validation and read-back. Their save methods now refuse to replace an already-present record that fails validation. This closes a low-level API path that could otherwise overwrite evidence of a corrupt configuration record.
- ESP32 #1 tracks whether geometry was loaded and validated from NVS for status/diagnostics only. If it is missing or corrupt, editable 80 cm / 800 cm RAM defaults remain usable for basic volume estimates and for range-checking the configured protection thresholds. The estimate must remain clearly identified as approximate; measured volume calibration is independent and may be accumulated gradually. Saving protection thresholds and requesting a well-source transfer do not require geometry to have been persisted or volume calibration to be complete. A well-source transfer still requires valid, configured pump-protection thresholds. After a successful geometry save/read-back, stored thresholds are revalidated against the updated usable depth.
- `NvsTransferJournalStore` previously fell back to the other valid slot when one slot was corrupt. This could roll back from a newer interrupted-transfer record to an older `CLEAR` record. It now fails closed if either present slot is corrupt, and refuses subsequent writes until service review. The host test was changed to assert no rollback and no automatic overwrite.
- The transfer-history and daily-inflow stores remain single-key records with CRC, validation, read-back and refusal to overwrite an already-corrupt record. They are informational/accounting stores, not substitutes for the transfer restart journal.
- Sensor calibration records are checksum-validated and the save path refuses to overwrite a corrupt existing record. Startup currently falls back to provisional RAM defaults when a sensor calibration is missing or invalid; this remains a distinct configuration-state issue for a later review and must not be confused with a validated installed-sensor calibration.
- Peer replay state remains a two-slot anti-rollback store with an initialization marker and fail-closed corruption policy.

### Verification after this follow-up

- `tests/run_native_tests.sh`: **59/59 native test programs PASS**, including the new geometry-store test and geometry startup/transfer integration check.
- `sh tests/.run_native_tests_asan.sh`: **59/59 native test programs PASS** under AddressSanitizer and UndefinedBehaviorSanitizer.
- Python unittest discovery: **17/17 PASS**.
- `python3 -m compileall -q tools tests/python`: PASS.
- Well geometry, well calibration and transfer journal source copies are byte-identical across `common`, ESP32 #1 and ESP32 #2.
- These are host-side checks. They do not prove Arduino Preferences/NVS atomicity or real power-cut recovery.

### Remaining limitations

- Arduino-ESP32 1.0.6 target compilation remains unavailable in this environment.
- No physical ESP32, sensor, pump or valve test was performed; no physical output was activated.
- No controlled flash power interruption, NVS encryption, or hardware recovery test was performed.
- The sensor-config startup fallback behavior still needs a dedicated review distinguishing a missing first-install calibration from a present-but-corrupt calibration.

## 7 Nové závazné pravidlo geometrie studny (7. 11. 2026)
- Geometrie není alternativní odhad, který může kalibrace nahradit nižší hodnotou. Je to minimální objemový model. Kalibrace může zachytit dutiny/rozšíření a výsledek pouze zvýšit.
- Implementace `estimateWellVolume()` proto v měřeném rozsahu vrací maximum geometrického minima a interpolované kalibrace. Mimo rozsah přenáší jen kladný přírůstek nad geometrií na nejbližším kalibračním bodě.
- Výchozí geometrie 80/800 cm v RAM zůstává použitelná při chybějícím či poškozeném NVS záznamu. Objem zůstává označen jako odhad. Chybějící objemová kalibrace ani neuložená geometrie samy o sobě neblokují základní provoz/transfer; nadále jsou vyžadovány samostatné bezpečnostní meze ochrany čerpadla a platné měření.
- Tento princip je závazný a nesmí být v dalších revizích změněn bez výslovného souhlasu uživatele.
