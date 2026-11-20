#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
for endpoint in firmware/ESP32_1/src firmware/ESP32_2/src; do
  for common_file in firmware/common/*; do
    [ -f "$common_file" ] || continue
    name=${common_file##*/}
    # Peer protocol/authentication modules are intentionally retired from both
    # Arduino sketch src/ trees. Their common copies remain only for historical
    # host tests/audit and are not part of either firmware build.
    case "$name" in
      Peer*|NvsPeer*|TimedPeer*|PersistentPeerReplayGuard*|HmacSha256*) continue ;;
    esac
    endpoint_file="$endpoint/$name"
    [ -f "$endpoint_file" ] || {
      echo "FAIL: $endpoint is missing common module $name" >&2
      exit 1
    }
    cmp "$common_file" "$endpoint_file" || {
      echo "FAIL: $endpoint/$name differs from firmware/common/$name" >&2
      exit 1
    }
  done
done
echo "PASS: all common module files are byte-identical in both ESP32 sketch source trees"
