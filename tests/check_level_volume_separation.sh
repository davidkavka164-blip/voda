#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
UPPER=firmware/ESP32_2/ESP32_2.ino
COMMON=firmware/common/SensorConfig.h

# The safety-facing API must describe the electrical level conversion, not litres.
grep -q 'sensorConfigUsableForLevelMeasurement' "$COMMON"
! grep -R -q 'sensorCalibrationTrustedForSafety' firmware

# Extract selected function bodies and assert they never depend on the well-volume curve.
python3 - "$MAIN" <<'PY'
from pathlib import Path
import sys
src = Path(sys.argv[1]).read_text()
functions = [
    'bool manualValveSafetyPermitted(',
    'void updateTransferControl(',
    'void updateLowerTankPumpProtection(',
    'void updateWellPumpProtection(',
]
for signature in functions:
    start = src.find(signature)
    if start < 0:
        raise SystemExit(f'MISSING function: {signature}')
    brace = src.find('{', start)
    depth = 0
    end = None
    for i in range(brace, len(src)):
        if src[i] == '{': depth += 1
        elif src[i] == '}':
            depth -= 1
            if depth == 0:
                end = i + 1
                break
    if end is None:
        raise SystemExit(f'UNTERMINATED function: {signature}')
    body = src[start:end]
    if 'wellCalibration' in body or 'estimateWellVolume' in body:
        raise SystemExit(f'VOLUME CALIBRATION LEAKS INTO SAFETY: {signature}')
print('PASS: well volume calibration is not consumed by pump, manual valve or transfer protection')
PY

# Upper-unit protection/peer validity depends on electrical level conversion and level sample only.
grep -q 'sensorConfigUsableForLevelMeasurement(upperSensorConfigState)' "$UPPER"
! grep -q 'upperVolumeValid.*upperPumpProtection' "$UPPER"
echo 'PASS: electrical level conversion and well volume estimate remain separate'
