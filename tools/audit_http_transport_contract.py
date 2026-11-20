#!/usr/bin/env python3
"""Static gate for the HWC dual-transport HTTP adapter contract (REV90)."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
EXPECTED = {
    "ESP32_1": {
        "/", "/geometry", "/calibration", "/sensor-calibration", "/sensor-recovery",
        "/protection-config", "/lower-pump-protection-config", "/lower-pump-emergency",
        "/transfer-start", "/transfer-stop", "/inflow-watchdog-config",
        "/transfer-recovery-ack", "/pump-emergency", "/manual-valve-mode",
        "/manual-valve-command", "/wifi", "/update",
    },
    "ESP32_2": {
        "/", "/local-limits", "/volume-anchor", "/volume-corrupt-reset",
        "/volume-point", "/volume-reference", "/volume-reference-cancel",
        "/volume-transfer", "/sensor-recovery", "/pump-emergency", "/update",
    },
}

failed = False
for unit, variable in (("ESP32_1", "webServer"), ("ESP32_2", "localWebServer")):
    source = (ROOT / "firmware" / unit / f"{unit}.ino").read_text(encoding="utf-8")
    routes = set(re.findall(r"\b" + variable + r"\.on\(\s*\"([^\"]+)\"\s*,\s*HTTP_(?:GET|POST)", source))
    methods = set(re.findall(r"\b" + variable + r"\.([A-Za-z_]\w*)\s*\(", source))
    missing = EXPECTED[unit] - routes
    extra = routes - EXPECTED[unit]
    print(f"{unit}: routes={len(routes)} expected={len(EXPECTED[unit])}; methods={','.join(sorted(methods))}")
    if missing or extra:
        failed = True
        print(f"  FAIL missing={sorted(missing)} extra={sorted(extra)}")
    if "collectHeaders" not in methods or "authenticate" not in methods or "requestAuthentication" not in methods:
        failed = True
        print("  FAIL authentication/header API surface changed")
    if "upload" not in methods:
        failed = True
        print("  FAIL OTA upload callback surface changed")

parser = (ROOT / "firmware" / "common" / "HttpRequestParser.h").read_text(encoding="utf-8")
for required in ("MAX_BODY = 1024", "PARSE_UNSUPPORTED_TRANSFER_ENCODING", "multipart/OTA", "MAX_HEADERS = 16"):
    if required not in parser:
        failed = True
        print(f"PARSER FAIL missing contract marker: {required}")

if failed:
    sys.exit(1)
print("PASS: 28 routes, authentication hooks, OTA upload hook and parser limits match REV90 baseline")
