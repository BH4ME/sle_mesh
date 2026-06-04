import os
import tempfile
import unittest
from unittest import mock

from automation.ws63.tools import ws63_auto_burn


class FakeSerial:
    def __init__(self):
        self.ops = []

    def write(self, data):
        self.ops.append(("write", data))
        return len(data)

    def flush(self):
        self.ops.append(("flush",))

    def reset_input_buffer(self):
        self.ops.append(("reset_input_buffer",))

    def setDTR(self, value):
        self.ops.append(("dtr", value))

    def setRTS(self, value):
        self.ops.append(("rts", value))


class Ws63AutoBurnTest(unittest.TestCase):
    def test_perform_auto_reset_sends_reboot_then_control_sequence(self):
        ser = FakeSerial()
        sleeps = []
        config = ws63_auto_burn.ResetConfig(
            command="reboot",
            fallback_command="",
            command_delay_s=0.25,
            command_retries=1,
            retry_gap_s=0.0,
            sequence=ws63_auto_burn.parse_control_sequence("rts=0,dtr=0:0.1;rts=0,dtr=1:0.2"),
        )

        ws63_auto_burn.perform_auto_reset(ser, config, sleep_fn=sleeps.append, log_fn=lambda message: None)

        self.assertEqual(
            ser.ops,
            [
                ("write", b"reboot\r\n"),
                ("flush",),
                ("rts", False),
                ("dtr", False),
                ("rts", False),
                ("dtr", True),
                ("reset_input_buffer",),
            ],
        )
        self.assertEqual(sleeps, [0.25, 0.1, 0.2])

    def test_perform_auto_reset_software_only_retries_commands(self):
        ser = FakeSerial()
        sleeps = []
        config = ws63_auto_burn.ResetConfig(
            command="reboot",
            fallback_command="reset",
            command_delay_s=0.1,
            command_retries=2,
            retry_gap_s=0.05,
            sequence=(),
        )

        ws63_auto_burn.perform_auto_reset(ser, config, sleep_fn=sleeps.append, log_fn=lambda message: None)

        self.assertEqual(
            ser.ops,
            [
                ("write", b"reboot\r\n"),
                ("flush",),
                ("write", b"reset\r\n"),
                ("flush",),
                ("write", b"reboot\r\n"),
                ("flush",),
                ("write", b"reset\r\n"),
                ("flush",),
                ("reset_input_buffer",),
            ],
        )
        self.assertEqual(sleeps, [0.1, 0.1, 0.05, 0.1, 0.1])

    def test_parse_control_sequence_rejects_unknown_signal(self):
        with self.assertRaises(ValueError):
            ws63_auto_burn.parse_control_sequence("boot=1:0.1")

    def test_show_mode_does_not_require_port(self):
        parser = ws63_auto_burn.build_arg_parser()

        args = parser.parse_args(["-s", "firmware.fwpkg"])

        self.assertTrue(args.show)
        self.assertIsNone(args.port)

    def test_parser_supports_software_reset_only_mode(self):
        parser = ws63_auto_burn.build_arg_parser()
        args = parser.parse_args(["-p", "/dev/null", "--software-reset-only", "firmware.fwpkg"])
        self.assertTrue(args.software_reset_only)

    def test_firmware_version_guard_checks_package_bytes(self):
        path = None
        try:
            with tempfile.NamedTemporaryFile(delete=False) as f:
                path = f.name
                f.write(b"boot data v4.4.57 app data")
                f.flush()

            self.assertTrue(ws63_auto_burn.firmware_contains_version(path, "v4.4.57"))
            self.assertFalse(ws63_auto_burn.firmware_contains_version(path, "v4.4.56"))
            self.assertTrue(ws63_auto_burn.firmware_contains_version(path, ""))
        finally:
            if path is not None:
                os.unlink(path)

    def test_main_refuses_stale_firmware_before_flash(self):
        path = None
        try:
            with tempfile.NamedTemporaryFile(delete=False) as f:
                path = f.name
                f.write(b"boot data v4.4.37 app data")
                f.flush()

            ret = ws63_auto_burn.main(["-p", "/dev/null", "--no-auto-reset", "--expected-version", "v4.4.57", path])
        finally:
            if path is not None:
                os.unlink(path)

        self.assertEqual(ret, 3)

    def test_main_returns_nonzero_when_flash_fails(self):
        class FailingBurner:
            def __init__(self, *_args, **_kwargs):
                pass

            def flash(self, _firmware):
                return False

        with mock.patch.object(ws63_auto_burn, "HAVE_XF_BURN_TOOLS", True), \
                mock.patch.object(ws63_auto_burn, "AutoResetWs63BurnTools", FailingBurner):
            ret = ws63_auto_burn.main([
                "-p", "/dev/null",
                "--no-auto-reset",
                "--expected-version", "",
                "firmware.fwpkg",
            ])

        self.assertNotEqual(ret, 0)

    def test_main_defaults_to_current_expected_firmware_version(self):
        parser = ws63_auto_burn.build_arg_parser()

        args = parser.parse_args(["-p", "/dev/null", "--software-reset-only", "firmware.fwpkg"])

        self.assertEqual(args.expected_version, "v4.4.57")

    def test_main_show_mode_does_not_apply_version_guard(self):
        class FakeFwpkg:
            def __init__(self, firmware_file):
                self.firmware_file = firmware_file

            def show(self):
                return None

        with tempfile.NamedTemporaryFile(delete=False) as f:
            path = f.name
            f.write(b"boot data v4.4.57 app data")
            f.flush()
        try:
            with mock.patch.object(ws63_auto_burn, "HAVE_XF_BURN_TOOLS", True), \
                    mock.patch.object(ws63_auto_burn, "Fwpkg", FakeFwpkg):
                ret = ws63_auto_burn.main(["--show", "--expected-version", "v4.4.57", path])
        finally:
            os.unlink(path)

        self.assertEqual(ret, 0)


if __name__ == "__main__":
    unittest.main()
