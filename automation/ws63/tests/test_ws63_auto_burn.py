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

    def test_main_returns_nonzero_when_flash_fails(self):
        class FailingBurner:
            def __init__(self, *_args):
                pass

            def flash(self, _firmware):
                return False

        with mock.patch.object(ws63_auto_burn, "AutoResetWs63BurnTools", FailingBurner):
            ret = ws63_auto_burn.main(["-p", "/dev/null", "--no-auto-reset", "firmware.fwpkg"])

        self.assertNotEqual(ret, 0)


if __name__ == "__main__":
    unittest.main()
