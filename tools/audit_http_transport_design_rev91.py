#!/usr/bin/env python3
"""Verify REV91 transport design against the actual production sketch API surface."""
from pathlib import Path
import re
import sys
ROOT = Path(__file__).resolve().parents[1]
checks = {
    'ESP32_1': ('webServer', 'requireWebAuth', 17),
    'ESP32_2': ('localWebServer', 'requireLocalAuth', 11),
}
failed = []
for unit, (var, guard, expected) in checks.items():
    src = (ROOT / 'firmware' / unit / f'{unit}.ino').read_text(encoding='utf-8')
    methods = set(re.findall(r'\b' + var + r'\.([A-Za-z_]\w*)\s*\(', src))
    routes = re.findall(r'\b' + var + r'\.on\(\s*"([^"]+)"\s*,\s*HTTP_(?:GET|POST)', src)
    required = {'arg','hasArg','send','authenticate','requestAuthentication','client','method','collectHeaders','upload','on','onNotFound','begin','handleClient'}
    missing = required - methods
    print(f'{unit}: routes={len(routes)}, required_api={len(required)}, missing={sorted(missing)}')
    if len(routes) != expected: failed.append(f'{unit}: route count {len(routes)} != {expected}')
    if missing: failed.append(f'{unit}: missing methods {sorted(missing)}')
    if f'{guard}(' not in src: failed.append(f'{unit}: auth guard missing')
    if not re.search(r'HTTP_POST\s*,\s*\w+\s*,\s*\w+\)', src): failed.append(f'{unit}: OTA completion/upload callback pair missing')
if failed:
    print('FAIL:', *failed, sep='\n - '); sys.exit(1)
print('PASS: design still matches both production WebServer surfaces and OTA callback structure')
