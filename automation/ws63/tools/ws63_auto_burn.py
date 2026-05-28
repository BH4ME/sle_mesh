#!/usr/bin/env python3
"""WS63 firmware burner with a project-level auto-reset hook."""

from __future__ import annotations

import argparse
import logging
import math
import sys
import time
from dataclasses import dataclass
from typing import Callable, Iterable, List, Optional

import serial
from xf_burn_tools.fwpkg import Fwpkg
from xf_burn_tools.pymodem import ymodem_xfer
from xf_burn_tools.ws63flash import (
    CMD_DOWNLOAD,
    CMD_HANDSHAKE,
    CMD_RST,
    RESET_TIMEOUT,
    UART_READ_TIMEOUT,
    WS63E_FLASHINFO,
    Ws63BurnTools,
)


SleepFn = Callable[[float], None]
LogFn = Callable[[str], None]


@dataclass(frozen=True)
class ControlStep:
    dtr: Optional[bool] = None
    rts: Optional[bool] = None
    delay_s: float = 0.0


@dataclass(frozen=True)
class ResetConfig:
    command: str = "reboot"
    fallback_command: str = "reset"
    compatibility_command: str = ""
    command_delay_s: float = 0.3
    command_retries: int = 2
    retry_gap_s: float = 0.2
    sequence: Iterable[ControlStep] = ()


DEFAULT_CONTROL_SEQUENCE = "rts=0,dtr=0:0.05;rts=0,dtr=1:0.12;rts=0,dtr=0:0.05"


def parse_bool(value: str) -> bool:
    normalized = value.strip().lower()
    if normalized in {"1", "true", "on", "yes", "high"}:
        return True
    if normalized in {"0", "false", "off", "no", "low"}:
        return False
    raise ValueError(f"invalid control value: {value}")


def parse_control_sequence(text: str) -> List[ControlStep]:
    steps: List[ControlStep] = []
    if not text.strip():
        return steps

    for raw_step in text.split(";"):
        raw_step = raw_step.strip()
        if not raw_step:
            continue
        controls, sep, delay_text = raw_step.partition(":")
        delay_s = float(delay_text) if sep else 0.0
        dtr: Optional[bool] = None
        rts: Optional[bool] = None
        for raw_part in controls.split(","):
            raw_part = raw_part.strip()
            if not raw_part:
                continue
            name, eq, value = raw_part.partition("=")
            if not eq:
                raise ValueError(f"missing '=' in control step: {raw_part}")
            name = name.strip().lower()
            parsed_value = parse_bool(value)
            if name == "dtr":
                dtr = parsed_value
            elif name == "rts":
                rts = parsed_value
            else:
                raise ValueError(f"unknown control signal: {name}")
        steps.append(ControlStep(dtr=dtr, rts=rts, delay_s=delay_s))
    return steps


def perform_auto_reset(
    ser,
    config: ResetConfig,
    *,
    sleep_fn: SleepFn = time.sleep,
    log_fn: LogFn = logging.info,
) -> None:
    command_list: List[str] = []
    for raw_command in (config.command, config.fallback_command, config.compatibility_command):
        command = raw_command.strip()
        if command and command not in command_list:
            command_list.append(command)

    retries = max(1, config.command_retries)
    for attempt in range(retries):
        for command in command_list:
            payload = command.encode("ascii") + b"\r\n"
            log_fn(f"Auto reset: sending CLI command '{command}'")
            ser.write(payload)
            ser.flush()
            if config.command_delay_s > 0.0:
                sleep_fn(config.command_delay_s)
        if attempt + 1 < retries and config.retry_gap_s > 0.0:
            sleep_fn(config.retry_gap_s)

    for step in config.sequence:
        parts = []
        if step.rts is not None:
            ser.setRTS(step.rts)
            parts.append(f"RTS={int(step.rts)}")
        if step.dtr is not None:
            ser.setDTR(step.dtr)
            parts.append(f"DTR={int(step.dtr)}")
        if parts:
            log_fn("Auto reset: " + " ".join(parts))
        if step.delay_s > 0.0:
            sleep_fn(step.delay_s)

    try:
        ser.reset_input_buffer()
    except serial.SerialException:
        logging.debug("Ignoring reset_input_buffer failure during auto reset", exc_info=True)


class AutoResetWs63BurnTools(Ws63BurnTools):
    def __init__(self, com: str, baudrate: int, reset_config: Optional[ResetConfig]) -> None:
        super().__init__(com, baudrate)
        self.reset_config = reset_config

    def flash(self, name) -> bool:  # noqa: C901 - mirrors xf_burn_tools to keep its protocol behavior.
        self.ser = serial.Serial(self.com, 115200, timeout=1)
        self.ser.setRTS(False)
        if self.reset_config is not None:
            perform_auto_reset(self.ser, self.reset_config)

        self.fwpkg = Fwpkg(name)
        loaderboot = None
        for bin_info in self.fwpkg.bin_infos:
            if bin_info["type"] == 0:
                loaderboot = bin_info
                break
        if not loaderboot:
            logging.error("Required loaderboot not found in fwpkg!")
            return False

        self.fwpkg.show()

        logging.info("Waiting for device reset...")
        t0 = time.time()
        while True:
            if time.time() - t0 > RESET_TIMEOUT:
                logging.warning("Timeout while waiting for device reset")
                return False
            WS63E_FLASHINFO[CMD_HANDSHAKE]["data"][0:4] = self.baudrate.to_bytes(4, "little")
            self.ws63SendCmddef(WS63E_FLASHINFO[CMD_HANDSHAKE])
            data = self.ser.read_all()
            ack = b"\xEF\xBE\xAD\xDE\x0C\x00\xE1\x1E"
            if ack in data:
                self.ser.baudrate = self.baudrate
                logging.info("Establishing ymodem session...")
                break

        time.sleep(0.5)
        logging.info(f"Transferring {loaderboot['name']}...")
        ret = ymodem_xfer(self.ser, name, loaderboot)
        if ret is False:
            logging.error(f"Error transferring {loaderboot['name']}")
            return False

        self.uartReadUntilMagic()

        for bin_info in self.fwpkg.bin_infos:
            if bin_info["type"] != 1:
                continue
            logging.info(f"Transferring {bin_info['name']}...")
            eras_size = math.ceil(bin_info["length"] / 8192.0) * 0x2000
            WS63E_FLASHINFO[CMD_DOWNLOAD]["data"][0:4] = bin_info["burn_addr"].to_bytes(4, "little")
            WS63E_FLASHINFO[CMD_DOWNLOAD]["data"][4:8] = bin_info["length"].to_bytes(4, "little")
            WS63E_FLASHINFO[CMD_DOWNLOAD]["data"][8:12] = int(eras_size).to_bytes(4, "little")
            self.ws63SendCmddef(WS63E_FLASHINFO[CMD_DOWNLOAD])
            self.uartReadUntilMagic()
            ret = ymodem_xfer(self.ser, name, bin_info)
            if ret is False:
                logging.error(f"Error transferring {bin_info['name']}")
                return False
            time.sleep(0.1)
        logging.info("Done. Reseting device...")
        self.ws63SendCmddef(WS63E_FLASHINFO[CMD_RST])
        self.uartReadUntilMagic()
        self.ser.close()
        return True


def build_arg_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="Burn WS63 firmware and try to reset the board automatically.")
    parser.add_argument("firmware_file")
    parser.add_argument("-p", "--port", help="serial port, for example /dev/tty.usbserial-10")
    parser.add_argument("-b", "--baudrate", type=int, default=115200, help="burn baudrate")
    parser.add_argument("-v", "--verbose", action="store_true", help="print debug logs")
    parser.add_argument("-s", "--show", action="store_true", help="show firmware information only")
    parser.add_argument("--no-auto-reset", action="store_true", help="disable CLI/DTR/RTS reset before burn")
    parser.add_argument("--reset-command", default="reboot", help="CLI command to send before handshaking")
    parser.add_argument("--reset-command-fallback", default="reset", help="fallback CLI command before handshaking")
    parser.add_argument(
        "--compat-reset-command",
        default="AT+RST",
        help="compatibility reset command for AT-style firmware",
    )
    parser.add_argument(
        "--no-compat-reset-command",
        action="store_true",
        help="do not send compatibility reset command",
    )
    parser.add_argument(
        "--no-reset-command-fallback",
        action="store_true",
        help="do not send fallback CLI reset command",
    )
    parser.add_argument("--no-reset-command", action="store_true", help="do not send a CLI reboot command")
    parser.add_argument("--reset-command-delay", type=float, default=0.3, help="delay after reset command, seconds")
    parser.add_argument(
        "--reset-command-retries",
        type=int,
        default=2,
        help="how many times to send reset command sequence before burn handshake",
    )
    parser.add_argument(
        "--reset-command-retry-gap",
        type=float,
        default=0.2,
        help="delay between repeated reset command sequences, seconds",
    )
    parser.add_argument(
        "--software-reset-only",
        action="store_true",
        help="do not drive DTR/RTS, use serial CLI reset commands only",
    )
    parser.add_argument(
        "--control-sequence",
        default=DEFAULT_CONTROL_SEQUENCE,
        help="DTR/RTS sequence, for example 'rts=0,dtr=1:0.1;rts=0,dtr=0:0.1'",
    )
    return parser


def main(argv: Optional[List[str]] = None) -> int:
    parser = build_arg_parser()
    args = parser.parse_args(argv)

    logging.basicConfig(level=logging.DEBUG if args.verbose else logging.INFO, format="%(message)s")

    if args.show:
        Fwpkg(args.firmware_file).show()
        return 0
    if not args.port:
        parser.error("the following arguments are required for burning: -p/--port")
    if args.reset_command_retries < 1:
        parser.error("--reset-command-retries must be >= 1")
    if args.reset_command_delay < 0.0:
        parser.error("--reset-command-delay must be >= 0")
    if args.reset_command_retry_gap < 0.0:
        parser.error("--reset-command-retry-gap must be >= 0")

    reset_config = None
    if not args.no_auto_reset:
        sequence = []
        if not args.software_reset_only:
            try:
                sequence = parse_control_sequence(args.control_sequence)
            except ValueError as exc:
                parser.error(str(exc))
        reset_config = ResetConfig(
            command="" if args.no_reset_command else args.reset_command,
            fallback_command="" if args.no_reset_command_fallback else args.reset_command_fallback,
            compatibility_command="" if args.no_compat_reset_command else args.compat_reset_command,
            command_delay_s=args.reset_command_delay,
            command_retries=args.reset_command_retries,
            retry_gap_s=args.reset_command_retry_gap,
            sequence=sequence,
        )

    tools = AutoResetWs63BurnTools(args.port, args.baudrate, reset_config)
    return 0 if tools.flash(args.firmware_file) else 1


if __name__ == "__main__":
    sys.exit(main())
