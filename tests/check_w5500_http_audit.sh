#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
AUDIT=docs/W5500_HTTP_DUAL_INTERFACE_AUDIT_REV86.md
for sketch in firmware/ESP32_1/ESP32_1.ino firmware/ESP32_2/ESP32_2.ino; do
  grep -Fq '#include <Ethernet.h>' "$sketch"
  grep -Fq '#include <WebServer.h>' "$sketch"
  ! grep -Eq 'WebServer_ESP32_W5500|SparkFun_WebServer_ESP32_W5500' "$sketch"
done
grep -Fq 'ESP32 core 2.0.6' "$AUDIT"
grep -Fq 'SPI a interrupt' "$AUDIT"
grep -Fq 'Basic Auth' "$AUDIT"
grep -Fq 'OTA uploadu' "$AUDIT"
grep -Fq 'bez změny produkčního firmware' "$AUDIT"
echo 'PASS: W5500 HTTP audit documents both current transports and blocks unverified stack replacement'
