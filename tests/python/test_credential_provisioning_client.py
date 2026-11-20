import os
import sys
import unittest
from unittest import mock
import io

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), "..", ".."))
sys.path.insert(0, os.path.join(ROOT, "tools"))

import credential_provisioning_client as client


class CredentialClientTests(unittest.TestCase):
    def test_crc_reference_vector(self):
        self.assertEqual(client.crc16_ccitt_false(b"123456789"), 0x29B1)

    def test_python_request_matches_cpp_protocol_golden_vector(self):
        frame = client.build_request_frame("HWC-TEST-1234", "AP-passphrase-2026", "Admin-secret-2026")
        expected = bytes.fromhex(
            "4857010100330d4857432d544553542d313233341241502d706173737068726173652d"
            "323032361141646d696e2d7365637265742d32303236ae41"
        )
        self.assertEqual(frame, expected)

    def test_cpp_result_matches_response_golden_vector(self):
        expected = bytes.fromhex("485701810001200782")
        self.assertEqual(client.parse_result_frame(expected), 0x20)

    def test_request_frame_layout_and_crc(self):
        frame = client.build_request_frame("HWC-LOWER", "lowerpass123", "Administrator123")
        self.assertEqual(frame[:2], b"HW")
        self.assertEqual(frame[2], 1)
        self.assertEqual(frame[3], 1)
        payload_length = (frame[4] << 8) | frame[5]
        self.assertEqual(payload_length, len(frame) - 8)
        self.assertEqual(len(frame), 6 + payload_length + 2)
        received = (frame[-2] << 8) | frame[-1]
        self.assertEqual(received, client.crc16_ccitt_false(frame[2:-2]))

    def test_maximum_lengths_produce_maximum_frame(self):
        frame = client.build_request_frame("A" * 32, "B" * 63, "C" * 64)
        self.assertEqual(len(frame), 170)

    def test_minimum_lengths_are_accepted(self):
        frame = client.build_request_frame("12345678", "12345678", "123456789012")
        self.assertEqual(len(frame), 6 + (1 + 8) + (1 + 8) + (1 + 12) + 2)

    def test_rejects_invalid_field_lengths(self):
        cases = [
            ("short", "12345678", "123456789012"),
            ("HWC-LOWER", "short", "123456789012"),
            ("HWC-LOWER", "12345678", "short"),
            ("A" * 33, "12345678", "123456789012"),
            ("HWC-LOWER", "B" * 64, "123456789012"),
            ("HWC-LOWER", "12345678", "C" * 65),
        ]
        for values in cases:
            with self.subTest(values=tuple(len(value) for value in values)):
                with self.assertRaises(client.ProtocolError):
                    client.build_request_frame(*values)

    def test_rejects_non_ascii_and_control_characters(self):
        with self.assertRaises(client.ProtocolError):
            client.build_request_frame("HWC-ČESKO", "12345678", "123456789012")
        with self.assertRaises(client.ProtocolError):
            client.build_request_frame("HWC-LOWER", "pass\nword1", "123456789012")

    def test_parse_success_result(self):
        crc_body = bytes((1, 0x81, 0, 1, 0x20))
        crc = client.crc16_ccitt_false(crc_body)
        result = b"HW" + crc_body + bytes((crc >> 8, crc & 0xFF))
        self.assertEqual(client.parse_result_frame(result), 0x20)

    def test_rejects_bad_result_crc(self):
        crc_body = bytes((1, 0x81, 0, 1, 0x20))
        crc = client.crc16_ccitt_false(crc_body)
        result = bytearray(b"HW" + crc_body + bytes((crc >> 8, crc & 0xFF)))
        result[-1] ^= 1
        with self.assertRaises(client.ProtocolError):
            client.parse_result_frame(result)

    def test_rejects_wrong_response_shape(self):
        with self.assertRaises(client.ProtocolError):
            client.parse_result_frame(b"HW")
        valid_crc_body = bytes((1, 0x82, 0, 1, 0x20))
        crc = client.crc16_ccitt_false(valid_crc_body)
        wrong_type = b"HW" + valid_crc_body + bytes((crc >> 8, crc & 0xFF))
        with self.assertRaises(client.ProtocolError):
            client.parse_result_frame(wrong_type)

    def test_exchange_waits_for_ready_before_sending(self):
        ready_body = bytes((1, 0x81, 0, 1, client.STATUS_SESSION_READY))
        ready_crc = client.crc16_ccitt_false(ready_body)
        ready = b"HW" + ready_body + bytes((ready_crc >> 8, ready_crc & 0xFF))
        result_body = bytes((1, 0x81, 0, 1, 0x20))
        result_crc = client.crc16_ccitt_false(result_body)
        result = b"HW" + result_body + bytes((result_crc >> 8, result_crc & 0xFF))

        class FakeSerial(object):
            def __init__(self):
                self.written = None
                self.flushed = False
                self.responses = bytearray(b"boot log\r\n" + ready + result)
            def write(self, data):
                self.written = bytes(data)
                return len(data)
            def flush(self):
                self.flushed = True
            def read(self, count):
                if not self.responses:
                    return b""
                value = self.responses[:count]
                del self.responses[:count]
                return bytes(value)

        fake = FakeSerial()
        frame = client.build_request_frame("HWC-LOWER", "lowerpass123", "Administrator123")
        self.assertEqual(client.exchange(fake, frame), 0x20)
        self.assertEqual(fake.written, frame)
        self.assertTrue(fake.flushed)

    def test_exchange_does_not_send_without_ready(self):
        failure_body = bytes((1, 0x81, 0, 1, 0x22))
        crc = client.crc16_ccitt_false(failure_body)
        failure = b"HW" + failure_body + bytes((crc >> 8, crc & 0xFF))

        class FakeSerial(object):
            def __init__(self):
                self.written = None
                self.responses = bytearray(failure)
            def write(self, data):
                self.written = bytes(data)
                return len(data)
            def flush(self):
                pass
            def read(self, count):
                value = self.responses[:count]
                del self.responses[:count]
                return bytes(value)

        fake = FakeSerial()
        with self.assertRaises(client.ProtocolError):
            client.exchange(fake, b"secret-frame")
        self.assertIsNone(fake.written)

    def test_status_reader_skips_noise_before_valid_frame(self):
        body = bytes((1, 0x81, 0, 1, client.STATUS_SESSION_READY))
        crc = client.crc16_ccitt_false(body)
        frame = b"HW" + body + bytes((crc >> 8, crc & 0xFF))

        class FakeSerial(object):
            def __init__(self):
                self.data = bytearray(b"sensor log\r\n")
                self.data.extend(frame)
            def read(self, count):
                value = self.data[:count]
                del self.data[:count]
                return bytes(value)

        self.assertEqual(client.read_protocol_status(FakeSerial()), client.STATUS_SESSION_READY)

    def test_status_text_contains_no_credentials(self):
        self.assertIn("uloženy", client.STATUS_TEXT[0x20])
        self.assertNotIn("password", " ".join(client.STATUS_TEXT.values()).lower())

    def test_preview_cli_does_not_echo_credentials(self):
        class TtyInput(io.StringIO):
            def isatty(self):
                return True

        secret_ap = "LowerSecret123"
        secret_admin = "AdminSecret98765"
        captured = io.StringIO()
        with mock.patch.object(client.sys, "stdin", TtyInput("")), \
             mock.patch("builtins.input", side_effect=["HWC-LOWER"]), \
             mock.patch.object(client.getpass, "getpass", side_effect=[secret_ap, secret_ap, secret_admin, secret_admin]), \
             mock.patch.object(client.sys, "stdout", captured):
            result = client.main([])
        self.assertEqual(result, 0)
        output = captured.getvalue()
        self.assertNotIn(secret_ap, output)
        self.assertNotIn(secret_admin, output)
        self.assertIn("NÁHLED", output)

    def test_send_is_blocked_without_firmware_integration_confirmation(self):
        class TtyInput(io.StringIO):
            def isatty(self):
                return True

        with mock.patch.object(client.sys, "stdin", TtyInput("")), \
             mock.patch.object(client.sys, "stderr", io.StringIO()), \
             mock.patch("builtins.input") as prompt:
            with self.assertRaises(SystemExit) as raised:
                client.main(["--send", "--port", "COM4"])
        self.assertEqual(raised.exception.code, 2)
        prompt.assert_not_called()


if __name__ == "__main__":
    unittest.main(verbosity=2)
