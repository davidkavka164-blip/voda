#!/bin/sh
set -eu
ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
cd "$ROOT"
MAIN=firmware/ESP32_1/ESP32_1.ino
UPPER=firmware/ESP32_2/ESP32_2.ino
if grep -Eq 'PeerStatusHmacCodec|NvsPeerSharedKeyStore|PeerStatusSessionCodec|TimedPeerHandshake|PeerStatus|peerUdp|WiFiUDP|EthernetUDP' "$MAIN" "$UPPER"; then
  echo 'FAIL: production sketches still contain inter-unit communication code.' >&2
  exit 1
fi
for endpoint in firmware/ESP32_1/src firmware/ESP32_2/src; do
  if find "$endpoint" -maxdepth 1 -type f -printf '%f\n' | grep -Eq '^(Peer|NvsPeer|TimedPeer|PersistentPeerReplayGuard|HmacSha256)'; then
    echo "FAIL: retired peer module remains in $endpoint" >&2
    exit 1
  fi
done
echo 'PASS: production firmware and Arduino src trees contain no peer authentication or inter-unit UDP path'
