#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
for SKETCH in firmware/ESP32_1/ESP32_1.ino firmware/ESP32_2/ESP32_2.ino; do
  grep -q '#include "src/WebAuthRateLimiter.h"' "$SKETCH"
  grep -q 'HWC::WebAuthRateLimiter webAuthRateLimiter;' "$SKETCH"
  awk '/bool requireWebAuth\(\)|bool requireLocalAuth\(\)/,/^}/ { print }' "$SKETCH" | grep -q 'webAuthRateLimiter.isBlocked(clientKey, now)'
  awk '/bool requireWebAuth\(\)|bool requireLocalAuth\(\)/,/^}/ { print }' "$SKETCH" | grep -q 'webAuthRateLimiter.recordFailure(clientKey, now)'
  awk '/bool requireWebAuth\(\)|bool requireLocalAuth\(\)/,/^}/ { print }' "$SKETCH" | grep -q 'webAuthRateLimiter.recordSuccess(clientKey)'
done
printf 'PASS: per-client HTTP auth limiter integrated into both sketches\n'
