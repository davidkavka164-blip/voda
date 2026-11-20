#!/bin/sh
set -eu
SKETCH="$(dirname "$0")/../firmware/ESP32_1/ESP32_1.ino"
grep -Fq 'Čerpadla běžně spouští a vypíná vlastní tlaková automatika; ESP32 přes samostatná bezpečnostní relé povoluje nebo přerušuje napájení čerpadel' "$SKETCH"
if grep -Fq 'Čerpadla nejsou firmwarově spínána' "$SKETCH"; then
  echo 'FAIL: dashboard still claims pump safety relays are not controlled by firmware' >&2
  exit 1
fi
echo 'PASS: dashboard accurately distinguishes pressure automation from ESP32 pump safety relays'
