# Web security audit – REV35

## Scope
Audit of the HTTP interfaces on ESP32 #1 and ESP32 #2, based on source inspection of REV34 and the REV35 changes.

## Findings

- Both dashboards (`GET /`) require administrator authentication.
- All registered state-changing handlers use `POST`; each handler checks administrator authentication. OTA upload callbacks also check authentication at upload start, and the completion handler checks it again.
- HTTP Basic Auth is used with generated administrator credentials and a per-client failed-login limiter (five failures in 60 seconds, then a 60-second block).
- Basic Auth is cached by browsers. A cross-site form could therefore attempt a state-changing request while the browser has credentials cached. REV34 did not have a same-origin request check.

## REV35 mitigation

Both web servers now collect the `Origin` and `Host` request headers. Before authentication is accepted for any POST, the firmware requires:

1. an explicit `Origin` header;
2. an explicit `Host` header;
3. an `http://` origin (the firmware currently serves plain HTTP);
4. an origin authority exactly matching the request Host value, case-insensitively; and
5. no path, backslash, query, fragment, or user-info in the origin authority.

Missing, malformed, cross-origin, and unverifiable POST requests are rejected with HTTP 403 before any handler changes state. The check applies to ordinary form submissions and OTA upload POSTs. This is a same-origin CSRF mitigation, not a replacement for authentication or transport encryption.

## Remaining limitations / operating guidance

- The web interfaces still use plain HTTP, not HTTPS. Administrator credentials and page contents are not encrypted on the local network. Use trusted Wi-Fi; do not expose either HTTP interface directly to the public internet.
- Remote administration should use a properly configured VPN (for example, the router VPN), not public HTTP port forwarding.
- Origin validation relies on browser-supplied Origin/Host headers. Non-browser clients must explicitly send a matching `Origin: http://<Host>` header for POST requests.
- Some legacy browsers or embedded clients that omit Origin on POST will be rejected intentionally.
- The guard does not protect against a malicious device already on the trusted LAN, compromised browser/device, or an attacker who can observe unencrypted traffic.
- No live ESP32 browser/OTA test was possible in this environment. Confirm normal dashboard forms and firmware upload from the intended Android/desktop browsers after flashing.

## Verification

- Static integration test added: `tests/check_web_csrf_origin_guard.sh`.
- Full native suite and Python tests are run for the release package; results are recorded in the REV35 release notes.
- Arduino-ESP32 target compilation and physical network/browser tests remain outstanding.

## Follow-up finding addressed in REV36

A separate OTA review found that ESP32 #2 checked the upper-pump emergency override at upload start and between chunks, but did not repeat the check immediately before `Update.end(true)`. REV36 adds this finalization-time check and aborts the OTA write if the override is active. This closes the identified timing gap in source logic; physical OTA interruption testing is still required.
