#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
INO=firmware/ESP32_1/ESP32_1.ino
STORE=firmware/common/WifiStaCredentialStore.h
grep -q '#include "src/WifiStaCredentialStore.h"' "$INO"
grep -q 'WifiStaCredentialStore wifiStore(wifiBackend)' "$INO"
if grep -q 'putString("ssid"\|putString("password"\|getString("ssid"\|getString("password"' "$INO"; then
  echo "FAIL: legacy split Wi-Fi access remains in production sketch" >&2
  exit 1
fi
for endpoint in firmware/ESP32_1/src/WifiStaCredentialStore.h firmware/ESP32_2/src/WifiStaCredentialStore.h; do
  cmp "$STORE" "$endpoint"
done
echo "PASS: ESP32 #1 uses transactional Wi-Fi STA storage; module copies are synchronized"
