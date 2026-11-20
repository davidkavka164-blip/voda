#!/bin/sh
set -eu
ROOT=$(CDPATH= cd -- "$(dirname "$0")/.." && pwd)
A="$ROOT/firmware/ESP32_1/ESP32_1.ino"
B="$ROOT/firmware/ESP32_2/ESP32_2.ino"
python3 - "$A" "$B" <<'PY'
import sys
from pathlib import Path
for filename, cache_flag, state_flag in [
    (sys.argv[1], "haveWellReading = false", "state = HWC::SENSOR_CONFIG_VALID_STORED"),
    (sys.argv[1], "haveLowerTankReading = false", "state = HWC::SENSOR_CONFIG_VALID_STORED"),
    (sys.argv[2], "haveUpperReading = false", "upperSensorConfigState = HWC::SENSOR_CONFIG_VALID_STORED"),
]:
    source = Path(filename).read_text()
    pos = source.find("void handleSensorRecovery()") if "ESP32_1.ino" in filename else source.find("void handleLocalSensorRecovery()")
    if pos < 0:
        raise SystemExit(f"missing recovery handler: {filename}")
    end_marker = "\nconst char* otaOpe" if "ESP32_1.ino" in filename else "\nvoid handleLocalLimits"
    end = source.find(end_marker, pos)
    if end < 0:
        raise SystemExit(f"could not delimit recovery handler: {filename}")
    block = source[pos:end]
    if cache_flag not in block or state_flag not in block:
        raise SystemExit(f"missing cache invalidation or trust-state transition: {filename} {cache_flag}")
    if block.index(cache_flag) > block.index(state_flag):
        raise SystemExit(f"cached provisional reading invalidated after trust transition: {filename} {cache_flag}")
print("PASS: service recovery invalidates cached provisional samples before marking sensor config trusted")
PY
