#!/usr/bin/env python3
"""Static inventory of the HWC WebServer API surface and route auth guards."""
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[1]
UNITS = [
    ("ESP32_1", ROOT / "firmware/ESP32_1/ESP32_1.ino", "webServer", "requireWebAuth"),
    ("ESP32_2", ROOT / "firmware/ESP32_2/ESP32_2.ino", "localWebServer", "requireLocalAuth"),
]


def function_body(source: str, name: str):
    match = re.search(r"\bvoid\s+" + re.escape(name) + r"\s*\([^)]*\)\s*\{", source)
    if not match:
        return None
    pos = match.end()
    depth = 1
    while pos < len(source) and depth:
        if source[pos] == "{":
            depth += 1
        elif source[pos] == "}":
            depth -= 1
        pos += 1
    return source[match.end():pos - 1] if depth == 0 else None


def main():
    failures = []
    total = 0
    for unit, path, server, guard in UNITS:
        source = path.read_text(encoding="utf-8")
        pattern = (re.escape(server) + r'\.on\("([^"]+)",\s*'
                   r'(HTTP_GET|HTTP_POST),\s*(\w+)(?:,\s*(\w+))?\)')
        routes = re.findall(pattern, source)
        if not routes:
            failures.append(f"{unit}: no registered routes found")
        print(f"{unit}: {len(routes)} routes")
        total += len(routes)
        for route, method, handler, upload in routes:
            body = function_body(source, handler)
            if body is None:
                failures.append(f"{unit} {method} {route}: handler {handler} not found")
            elif guard not in body:
                failures.append(f"{unit} {method} {route}: {handler} lacks {guard}()")
            if route == "/update" and not upload:
                failures.append(f"{unit}: OTA route has no upload callback")
            print(f"  {method:8s} {route:30s} {handler}" + (f" + {upload}" if upload else ""))
        if source.count(f"{server}.onNotFound") != 1:
            failures.append(f"{unit}: expected exactly one authenticated onNotFound registration")
        if f"{server}.handleClient()" not in source:
            failures.append(f"{unit}: missing cooperative handleClient() call")
    if total != 28:
        failures.append(f"expected 28 total routes, found {total}")
    if failures:
        print("FAIL:")
        for failure in failures:
            print(" - " + failure)
        return 1
    print("PASS: 28 registered routes, every route callback has its unit auth guard, OTA callbacks present")
    return 0


if __name__ == "__main__":
    sys.exit(main())
