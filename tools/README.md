# Credential provisioning client

This is a host-side protocol client for the HWC local credential provisioning frame. See `../docs/SECURITY_CREDENTIAL_HOST_CLIENT_0.92.md` before use.

- Python 3.6 or newer.
- Preview is the default and sends nothing.
- PySerial 3.5 is needed only for a future serial send.
- **The current firmware does not integrate the device-side serial/GPIO adapter. Do not use `--send` with DEV KIT 0.92.**
- Never place passwords in command-line arguments, scripts, environment variables, tickets, or logs.
