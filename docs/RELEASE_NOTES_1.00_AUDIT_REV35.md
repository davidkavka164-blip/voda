# HOME WATER CONTROLLER – audit draft REV35

## Changes

- Added same-origin validation to all state-changing HTTP POST requests on ESP32 #1 and ESP32 #2.
- Both WebServer instances collect `Origin` and `Host` headers.
- Missing or mismatched origin metadata is rejected with HTTP 403 before authentication/handler execution.
- Added `tests/check_web_csrf_origin_guard.sh` to the native/integration test runner.
- Added `docs/WEB_SECURITY_AUDIT_REV35.md` with findings, mitigation and remaining limitations.

## Security note

This reduces cross-site request-forgery risk with browser-cached HTTP Basic Auth. It does not encrypt HTTP. Do not publish the web interfaces directly to the internet; use VPN for remote access.

## Verification status

- Native suite: PASS, 65 test programs plus all static/integration checks, including the new Origin/Host guard.
- Python suite: PASS, 17/17 tests.
- Python bytecode compilation: PASS.
- Shell syntax check (`bash -n tests/*.sh`): PASS.
- Arduino-ESP32 target compilation: NOT RUN (Arduino CLI/toolchain is not available in this environment).
- Physical browser, CSRF, Wi-Fi and OTA tests: NOT RUN; these require the actual ESP32 units and intended client browsers.
