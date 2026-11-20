#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""HOME WATER CONTROLLER local credential provisioning client.

Python 3.6+; pySerial is required only for an actual serial send.
The current sketch .ino files do not wire the board adapter to a physical GPIO/UART.
"""
from __future__ import print_function

import argparse
import getpass
import sys
import time

MAGIC = b"HW"
PROTOCOL_VERSION = 1
COMMAND_SET_CREDENTIALS = 0x01
RESPONSE_RESULT = 0x81
STATUS_SESSION_READY = 0x01
RESPONSE_SIZE = 9
DEFAULT_BAUD = 115200

STATUS_TEXT = {
    0x12: "Zařízení odmítlo chybný rámec.",
    0x20: "Přístupové údaje byly uloženy.",
    0x21: "Servisní relace není otevřená.",
    0x22: "Servisní autorizace vypršela.",
    0x23: "Zařízení odmítlo neplatné přístupové údaje.",
    0x24: "Chyba čtení nebo zápisu úložiště NVS.",
    0x25: "Obnova poškozeného úložiště se nezdařila.",
    0x26: "Zápis přístupových údajů se nezdařil.",
}


class ProtocolError(ValueError):
    pass


def crc16_ccitt_false(data):
    """CRC-16/CCITT-FALSE: poly=0x1021, init=0xFFFF, xorout=0."""
    crc = 0xFFFF
    for value in bytearray(data):
        crc ^= value << 8
        for unused in range(8):
            if crc & 0x8000:
                crc = ((crc << 1) ^ 0x1021) & 0xFFFF
            else:
                crc = (crc << 1) & 0xFFFF
    return crc


def _encode_field(value, field_name, minimum, maximum):
    if not isinstance(value, str):
        raise ProtocolError("{} musí být text.".format(field_name))
    try:
        encoded = value.encode("ascii")
    except UnicodeEncodeError:
        raise ProtocolError("{} smí obsahovat pouze tisknutelné ASCII znaky.".format(field_name))
    if len(encoded) < minimum or len(encoded) > maximum:
        raise ProtocolError("{} musí mít {} až {} znaků.".format(field_name, minimum, maximum))
    if any(byte < 0x20 or byte > 0x7E for byte in bytearray(encoded)):
        raise ProtocolError("{} obsahuje nepovolený řídicí znak.".format(field_name))
    return bytes((len(encoded),)) + encoded


def build_request_frame(ap_name, ap_password, admin_password):
    """Build SET_CREDENTIALS frame matching CredentialProvisioningSerialParser."""
    payload = (
        _encode_field(ap_name, "Název AP", 8, 32)
        + _encode_field(ap_password, "Heslo AP", 8, 63)
        + _encode_field(admin_password, "Heslo správce", 12, 64)
    )
    if len(payload) > 162:
        raise ProtocolError("Payload překračuje povolenou délku.")
    header = MAGIC + bytes((PROTOCOL_VERSION, COMMAND_SET_CREDENTIALS)) + len(payload).to_bytes(2, "big")
    protected = header[2:] + payload
    crc = crc16_ccitt_false(protected)
    return header + payload + bytes(((crc >> 8) & 0xFF, crc & 0xFF))


def parse_result_frame(frame):
    """Validate the fixed 9-byte device result frame and return its status."""
    raw = bytes(frame)
    if len(raw) != RESPONSE_SIZE:
        raise ProtocolError("Odpověď zařízení nemá přesně 9 bajtů.")
    if raw[0:2] != MAGIC:
        raise ProtocolError("Odpověď nemá platnou synchronizační značku HW.")
    if raw[2] != PROTOCOL_VERSION or raw[3] != RESPONSE_RESULT:
        raise ProtocolError("Odpověď má neznámou verzi nebo typ.")
    if raw[4:6] != b"\x00\x01":
        raise ProtocolError("Odpověď má neplatnou délku payloadu.")
    received_crc = (raw[7] << 8) | raw[8]
    if received_crc != crc16_ccitt_false(raw[2:7]):
        raise ProtocolError("Kontrolní součet odpovědi nesouhlasí.")
    return raw[6]


def read_protocol_status(serial_connection, max_noise_bytes=2048):
    """Find one valid 9-byte HW status frame while tolerating unrelated boot logs."""
    window = bytearray()
    for unused in range(max_noise_bytes + RESPONSE_SIZE):
        value = serial_connection.read(1)
        if not value:
            raise IOError("Zařízení nevrátilo odpověď. Ověř servisní relaci a firmware.")
        window.extend(value)
        if len(window) > RESPONSE_SIZE:
            del window[0]
        if len(window) == RESPONSE_SIZE and window[0:2] == MAGIC:
            try:
                return parse_result_frame(window)
            except ProtocolError:
                # Slide by one byte: the apparent marker may have been part of
                # ordinary Serial Monitor text, before a real frame follows.
                continue
    raise IOError("V sériovém přenosu nebyl nalezen platný rámec zařízení.")


def exchange(serial_connection, frame):
    """Wait for physical-session READY, send one request, then read its result."""
    ready_status = read_protocol_status(serial_connection)
    if ready_status != STATUS_SESSION_READY:
        raise ProtocolError("Zařízení nepotvrdilo fyzicky autorizovanou servisní relaci; hesla nebyla odeslána.")
    written = serial_connection.write(frame)
    if written != len(frame):
        raise IOError("Sériový port nepřijal celý rámec.")
    serial_connection.flush()
    status = read_protocol_status(serial_connection)
    if status == STATUS_SESSION_READY:
        raise ProtocolError("Zařízení opakovaně oznámilo READY místo výsledku operace.")
    return status


def _open_serial(port, baud):
    try:
        import serial
    except ImportError:
        raise RuntimeError("Chybí pySerial. Nainstaluj závislost příkazem: python -m pip install -r tools/requirements.txt")

    # Configure DTR/RTS before opening. This reduces, but cannot guarantee the
    # absence of, board-specific auto-reset pulses. The actual board must be tested.
    connection = serial.Serial()
    connection.port = port
    connection.baudrate = baud
    connection.timeout = 8
    connection.write_timeout = 3
    connection.xonxoff = False
    connection.rtscts = False
    connection.dsrdtr = False
    connection.dtr = False
    connection.rts = False
    connection.open()
    return connection


def _read_credentials():
    ap_name = input("Název konfiguračního AP (8–32 ASCII znaků): ")
    ap_password = getpass.getpass("Nové heslo AP (8–63 znaků): ")
    ap_password_confirm = getpass.getpass("Potvrď heslo AP: ")
    if ap_password != ap_password_confirm:
        raise ProtocolError("Hesla AP se neshodují.")
    admin_password = getpass.getpass("Nové heslo správce (12–64 znaků): ")
    admin_password_confirm = getpass.getpass("Potvrď heslo správce: ")
    if admin_password != admin_password_confirm:
        raise ProtocolError("Hesla správce se neshodují.")
    return ap_name, ap_password, admin_password


def main(argv=None):
    parser = argparse.ArgumentParser(
        description="Lokální klient pro protokol HWC SET_CREDENTIALS (HW v1)."
    )
    parser.add_argument("--send", action="store_true", help="odeslat rámec přes místní sériový port")
    parser.add_argument("--port", help="sériový port, např. COM4 nebo /dev/ttyUSB0")
    parser.add_argument("--baud", type=int, default=DEFAULT_BAUD, help="rychlost portu (výchozí 115200)")
    parser.add_argument(
        "--firmware-integrated", action="store_true",
        help="potvrzení, že cílový firmware již obsahuje a testuje Serial/GPIO adapter"
    )
    args = parser.parse_args(argv)

    if args.baud <= 0:
        parser.error("--baud musí být kladné číslo")
    if args.send and not args.port:
        parser.error("Při --send je nutné zadat --port")
    if args.send and not args.firmware_integrated:
        parser.error("Odeslání je blokováno. Použij --firmware-integrated až po skutečné integraci a ověření adaptéru v cílovém firmware.")
    if not args.send and (args.port or args.firmware_integrated):
        parser.error("--port a --firmware-integrated mají význam pouze spolu s --send")
    if not sys.stdin.isatty():
        parser.error("Interaktivní zadávání hesel vyžaduje skutečný terminál.")

    credentials = None
    frame = None
    connection = None
    try:
        credentials = _read_credentials()
        frame = bytearray(build_request_frame(*credentials))
        print("Vstupy jsou platné. Připravený rámec má {} bajtů; hesla ani rámec se nevypisují.".format(len(frame)))

        if not args.send:
            print("NÁHLED: nic nebylo odesláno ani uloženo do zařízení.")
            return 0

        print("UPOZORNĚNÍ: protokol posílá hesla v plaintextu po přímém místním kabelu.")
        print("Nepoužívej Bluetooth, sériový server ani síťový převodník.")
        print("Aktuální sketch .ino ještě nemají adapter připojený ke konkrétnímu GPIO/UART; odesílej pouze do skutečně integrovaného a ověřeného firmware.")
        confirmation = input("Pro pokračování napiš přesně SEND: ")
        if confirmation != "SEND":
            print("Odeslání zrušeno.")
            return 2

        connection = _open_serial(args.port, args.baud)
        connection.reset_input_buffer()
        input("Nyní proveď fyzické servisní podržení na zařízení alespoň 3 s. Po dokončení stiskni Enter (nejvýše do 60 s): ")
        print("Čekám na potvrzení READY ze zařízení; hesla budou odeslána až poté.")
        status = exchange(connection, frame)
        print(STATUS_TEXT.get(status, "Zařízení vrátilo neznámý stav 0x{:02X}.".format(status)))
        return 0 if status == 0x20 else 1
    except (ProtocolError, IOError, OSError, RuntimeError) as exc:
        print("Chyba: {}".format(exc), file=sys.stderr)
        return 1
    finally:
        if connection is not None:
            try:
                connection.close()
            except Exception:
                pass
        if frame is not None:
            for index in range(len(frame)):
                frame[index] = 0
        # Python strings cannot be reliably zeroized; process memory clearing is
        # best-effort only. The tool never writes credentials to files or logs.
        credentials = None


if __name__ == "__main__":
    sys.exit(main())
