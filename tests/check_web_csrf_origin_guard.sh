#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for SPEC in "firmware/ESP32_1/ESP32_1.ino:webServer:requireWebAuth" "firmware/ESP32_2/ESP32_2.ino:localWebServer:requireLocalAuth"; do
  FILE=${SPEC%%:*}; REST=${SPEC#*:}; SERVER=${REST%%:*}; AUTH=${REST#*:}
  grep -q '#include "src/HttpOriginPolicy.h"' "$FILE"
  grep -q 'HWC::isSameHttpOriginAuthority' "$FILE"
  grep -q "${SERVER}.collectHeaders(requestHeaders, 2)" "$FILE"
  awk "/bool $AUTH\(\)/,/^}/" "$FILE" | grep -q "$SERVER.method() == HTTP_POST && !isSameOriginPost($SERVER)"
  awk "/bool $AUTH\(\)/,/^}/" "$FILE" | grep -q 'send(403'
  grep -Fq "${SERVER}.on(\"/\", HTTP_GET" "$FILE"
done
for FILE in firmware/common/HttpOriginPolicy.h firmware/ESP32_1/src/HttpOriginPolicy.h firmware/ESP32_2/src/HttpOriginPolicy.h; do
  cmp -s firmware/common/HttpOriginPolicy.h "$FILE"
done
printf 'PASS: shared strict same-origin policy integrated into both web interfaces\n'
