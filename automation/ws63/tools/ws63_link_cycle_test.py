#!/usr/bin/env python3
"""WS63 serial member lifecycle test.

Automates the two priority live-board cases:
1) member online -> member reboot/signal loss -> leader sees offline -> member auto-restores and rejoins
2) member online -> manual leave -> leader sees offline -> member stays idle -> manual rejoin succeeds
"""

from __future__ import annotations

import argparse
import pathlib
import re
import time
from dataclasses import dataclass, field
from typing import Iterable, Optional

try:
    import serial  # type: ignore
except ImportError as exc:  # pragma: no cover
    raise SystemExit("pyserial is required: pip install pyserial") from exc


@dataclass
class Peer:
    name: str
    port: str
    baudrate: int
    ser: serial.Serial
    log: list[str] = field(default_factory=list)

    def send_line(self, line: str) -> None:
        payload = (line + "\r\n").encode("utf-8")
        self.ser.write(payload)
        self.ser.flush()
        self.log.append(f"[tx] {line}")


def _route_id_from_suffix(suffix: int) -> int:
    route_id = suffix & 0xFF
    if route_id == 0 or route_id == 0xFF:
        route_id = ((suffix >> 8) % 254) + 1
    return route_id


def _extract_suffix(text: str) -> Optional[int]:
    label_match = re.search(r"label=[ULM]([0-9A-Fa-f]{4})", text)
    if label_match:
        return int(label_match.group(1), 16)

    json_label_match = re.search(r"selfLabel\"?:\"?[ULM]([0-9A-Fa-f]{4})", text)
    if json_label_match:
        return int(json_label_match.group(1), 16)

    mac_match = re.search(
        r"mac=[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:[0-9A-Fa-f]{2}:([0-9A-Fa-f]{2}):([0-9A-Fa-f]{2})",
        text,
    )
    if mac_match:
        return int(mac_match.group(1) + mac_match.group(2), 16)

    return None


def _open_peer(name: str, port: str, baudrate: int) -> Peer:
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = baudrate
    ser.timeout = 0.1
    # Keep control lines inactive; several CH340-wired WS63 boards reset or
    # remain quiet when pyserial opens with default DTR/RTS assertion.
    ser.dtr = False
    ser.rts = False
    ser.open()
    ser.dtr = False
    ser.rts = False
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    return Peer(name=name, port=port, baudrate=baudrate, ser=ser)


def _read_once(peer: Peer) -> str:
    data = peer.ser.read(512)
    if not data:
        return ""
    text = data.decode("utf-8", errors="ignore")
    peer.log.append(text)
    return text


def _drain(peers: Iterable[Peer], seconds: float) -> None:
    end = time.time() + seconds
    while time.time() < end:
        for p in peers:
            _read_once(p)
        time.sleep(0.02)


def _wait_regex(target: Peer, peers: Iterable[Peer], pattern: str, timeout_s: float, note: str) -> None:
    rx = re.compile(pattern)
    end = time.time() + timeout_s
    buf = ""
    while time.time() < end:
        for p in peers:
            chunk = _read_once(p)
            if p is target and chunk:
                buf += chunk
                if rx.search(buf):
                    return
        time.sleep(0.02)
    raise RuntimeError(f"timeout waiting for {note}: pattern={pattern}")


def _wait_and_extract_suffix(target: Peer, peers: Iterable[Peer], timeout_s: float, note: str) -> int:
    end = time.time() + timeout_s
    buf = ""
    while time.time() < end:
        for p in peers:
            chunk = _read_once(p)
            if p is target and chunk:
                buf += chunk
                suffix = _extract_suffix(buf)
                if suffix is not None:
                    return suffix
        time.sleep(0.02)
    raise RuntimeError(f"timeout waiting for {note}: cannot parse MAC suffix from serial output")


def _query_role(peer: Peer, peers: Iterable[Peer], query_window_s: float) -> Optional[int]:
    peer.send_line("state")
    end = time.time() + query_window_s
    buf = ""
    while time.time() < end:
        for p in peers:
            chunk = _read_once(p)
            if p is peer and chunk:
                buf += chunk
        time.sleep(0.02)

    role: Optional[int] = None
    for m in re.finditer(r"\brole=(\d+)\b", buf):
        role = int(m.group(1))
    return role


def _wait_role(peer: Peer, peers: Iterable[Peer], expected_role: int, timeout_s: float, poll_s: float) -> None:
    end = time.time() + timeout_s
    while time.time() < end:
        role = _query_role(peer, peers, query_window_s=min(0.8, poll_s))
        if role == expected_role:
            return
        time.sleep(max(0.1, poll_s - 0.8))
    raise RuntimeError(f"{peer.name} role did not reach {expected_role} in {timeout_s}s")


def _query_member_online(leader: Peer, peers: Iterable[Peer], member_id: int, query_window_s: float) -> Optional[int]:
    leader.send_line("members")
    end = time.time() + query_window_s
    buf = ""
    while time.time() < end:
        for p in peers:
            chunk = _read_once(p)
            if p is leader and chunk:
                buf += chunk
        time.sleep(0.02)

    match_iter = re.finditer(r"member=(\d+).*?online=(\d+)", buf)
    state: Optional[int] = None
    for m in match_iter:
        if int(m.group(1)) == member_id:
            state = int(m.group(2))
    return state


def _wait_member_state(
    leader: Peer,
    peers: Iterable[Peer],
    member_id: int,
    expect_online: int,
    timeout_s: float,
    poll_s: float,
) -> None:
    end = time.time() + timeout_s
    while time.time() < end:
        state = _query_member_online(leader, peers, member_id, query_window_s=min(0.6, poll_s))
        if expect_online == 1:
            if state == 1:
                return
        else:
            # Treat missing member or online=0 as offline from leader perspective.
            if state is None or state == 0:
                return
        time.sleep(max(0.1, poll_s - 0.6))
    raise RuntimeError(f"member={member_id} did not reach online={expect_online} in {timeout_s}s")


def _detect_suffix(peer: Peer, peers: Iterable[Peer], provided_hex: str, timeout_s: float, note: str) -> int:
    if provided_hex:
        return int(provided_hex, 16) & 0xFFFF
    peer.send_line("wifi")
    return _wait_and_extract_suffix(peer, peers, timeout_s=timeout_s, note=note)


def _bootstrap_roles(
    leader: Peer,
    member: Peer,
    peers: Iterable[Peer],
    *,
    leader_suffix: int,
    bootstrap_timeout_s: float,
    poll_interval_s: float,
) -> int:
    leader_id = _route_id_from_suffix(leader_suffix)

    leader.send_line("role leader")
    _wait_regex(leader, peers, r"role leader ret=0", bootstrap_timeout_s, "set leader role")

    member.send_line(f"role member {leader_suffix:04X}")
    _wait_regex(member, peers, r"role member leader_suffix=[0-9A-Fa-f]{4} .*ret=0", bootstrap_timeout_s,
                "set member role")

    _wait_role(leader, peers, expected_role=1, timeout_s=bootstrap_timeout_s, poll_s=poll_interval_s)
    _wait_role(member, peers, expected_role=0, timeout_s=bootstrap_timeout_s, poll_s=poll_interval_s)
    return leader_id


def _pairing_join(
    leader: Peer,
    member: Peer,
    peers: Iterable[Peer],
    *,
    team_id: int,
    leader_id: int,
    member_id: int,
    channel: int,
    cmd_timeout_s: float,
    state_timeout_s: float,
    poll_interval_s: float,
    note: str,
) -> None:
    leader.send_line("pairing start")
    _wait_regex(leader, peers, r"pairing start ret=0", cmd_timeout_s, f"pairing start({note})")

    member.send_line(f"join {team_id} {leader_id} {channel}")
    _wait_regex(member, peers, r"join team=.* ret=0", cmd_timeout_s, f"member join({note})")

    leader.send_line("pairing stop")
    _wait_regex(leader, peers, r"pairing stop ret=0", cmd_timeout_s, f"pairing stop({note})")
    _wait_member_state(leader, peers, member_id, 1, state_timeout_s, poll_interval_s)


def _role_member_rejoin(
    leader: Peer,
    member: Peer,
    peers: Iterable[Peer],
    *,
    leader_suffix: int,
    member_id: int,
    cmd_timeout_s: float,
    state_timeout_s: float,
    poll_interval_s: float,
) -> None:
    member.send_line(f"role member {leader_suffix:04X}")
    _wait_regex(member, peers, r"role member leader_suffix=[0-9A-Fa-f]{4} .*ret=0", cmd_timeout_s,
                "manual role member rejoin")
    _wait_role(member, peers, expected_role=0, timeout_s=state_timeout_s, poll_s=poll_interval_s)
    _wait_member_state(leader, peers, member_id, 1, state_timeout_s, poll_interval_s)


def _wait_leader_offline_event(
    leader: Peer,
    peers: Iterable[Peer],
    *,
    member_id: int,
    timeout_s: float,
    note: str,
    log_start: int,
) -> None:
    pattern = rf"(member offline id={member_id}\b|member heartbeat timeout)"
    rx = re.compile(pattern)
    end = time.time() + timeout_s
    while time.time() < end:
        if rx.search("".join(leader.log[log_start:])):
            return
        for p in peers:
            _read_once(p)
        time.sleep(0.02)
    raise RuntimeError(f"timeout waiting for {note}: pattern={pattern}")


def _dump_logs(peers: Iterable[Peer], out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for p in peers:
        path = out_dir / f"{p.name}_{pathlib.Path(p.port).name}.log"
        with path.open("w", encoding="utf-8", errors="ignore") as f:
            for item in p.log:
                f.write(item)


def run(args: argparse.Namespace) -> int:
    peers: list[Peer] = []
    try:
        leader = _open_peer("leader", args.leader_port, args.baudrate)
        member = _open_peer("member", args.member_port, args.baudrate)
        peers = [leader, member]

        _drain(peers, args.initial_drain_s)

        leader_suffix = _detect_suffix(
            leader,
            peers,
            args.leader_suffix,
            args.bootstrap_timeout_s,
            "leader wifi status",
        )
        resolved_leader_id = args.leader_id
        if args.bootstrap_roles:
            resolved_leader_id = _bootstrap_roles(
                leader,
                member,
                peers,
                leader_suffix=leader_suffix,
                bootstrap_timeout_s=args.bootstrap_timeout_s,
                poll_interval_s=args.poll_interval_s,
            )
            if args.leader_id > 0 and args.leader_id != resolved_leader_id:
                print(
                    "[link-cycle] WARN: provided --leader-id=%d differs from derived leader_id=%d, use derived id"
                    % (args.leader_id, resolved_leader_id)
                )

        if resolved_leader_id <= 0:
            resolved_leader_id = _route_id_from_suffix(leader_suffix)

        if args.bootstrap_roles:
            _wait_member_state(leader, peers, args.member_id, 1, args.state_timeout_s, args.poll_interval_s)
        else:
            _pairing_join(
                leader,
                member,
                peers,
                team_id=args.team_id,
                leader_id=resolved_leader_id,
                member_id=args.member_id,
                channel=args.channel,
                cmd_timeout_s=args.cmd_timeout_s,
                state_timeout_s=args.state_timeout_s,
                poll_interval_s=args.poll_interval_s,
                note="initial",
            )

        if not args.skip_reboot_cycle:
            leader_offline_log_start = len(leader.log)
            member.send_line(args.member_reboot_command)
            _wait_leader_offline_event(
                leader,
                peers,
                member_id=args.member_id,
                timeout_s=args.reboot_offline_timeout_s,
                note="leader offline after member reboot",
                log_start=leader_offline_log_start,
            )
            _wait_regex(
                member,
                peers,
                r"\[team-nv\] (load role=0 .*|restore member leader_suffix=)",
                args.member_boot_timeout_s,
                "member restore from NV after reboot",
            )
            _wait_member_state(leader, peers, args.member_id, 1, args.state_timeout_s, args.poll_interval_s)

        leader_offline_log_start = len(leader.log)
        member.send_line("leave")
        _wait_regex(member, peers, r"leave ret=0", args.cmd_timeout_s, "member leave")
        _wait_leader_offline_event(
            leader,
            peers,
            member_id=args.member_id,
            timeout_s=args.state_timeout_s,
            note="leader offline after manual leave",
            log_start=leader_offline_log_start,
        )
        if args.no_auto_rejoin_s > 0:
            _drain(peers, args.no_auto_rejoin_s)
            state = _query_member_online(leader, peers, args.member_id, query_window_s=args.poll_interval_s)
            if state == 1:
                raise RuntimeError("member auto-rejoined after manual leave")

        _role_member_rejoin(
            leader,
            member,
            peers,
            leader_suffix=leader_suffix,
            member_id=args.member_id,
            cmd_timeout_s=args.cmd_timeout_s,
            state_timeout_s=args.state_timeout_s,
            poll_interval_s=args.poll_interval_s,
        )

        print("[link-cycle] PASS: reboot restore + manual leave/rejoin")
        return 0
    except Exception as exc:  # noqa: BLE001
        print(f"[link-cycle] FAIL: {exc}")
        return 1
    finally:
        if args.log_dir:
            _dump_logs(peers, pathlib.Path(args.log_dir))
        for p in peers:
            try:
                p.ser.close()
            except Exception:  # noqa: BLE001
                pass


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="WS63 link cycle serial smoke test")
    parser.add_argument("--leader-port", required=True, help="leader serial port, e.g. /dev/tty.usbserial-10")
    parser.add_argument("--member-port", required=True, help="member serial port, e.g. /dev/tty.usbserial-110")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--bootstrap-roles", action="store_true", help="auto-configure leader/member roles before link cycle")
    parser.add_argument(
        "--leader-suffix",
        default="",
        help="optional leader MAC suffix hex (4 chars), e.g. C7E9; auto-detect from serial if omitted",
    )
    parser.add_argument("--bootstrap-timeout-s", type=float, default=20.0)
    parser.add_argument("--team-id", type=int, default=1)
    parser.add_argument("--leader-id", type=int, default=-1, help="leader route id; <=0 means auto (derived when bootstrapping)")
    parser.add_argument("--member-id", type=int, default=2)
    parser.add_argument("--channel", type=int, default=17)
    parser.add_argument("--skip-reboot-cycle", action="store_true", help="skip member reboot/auto-restore validation")
    parser.add_argument("--member-reboot-command", default="reboot", help="serial command used to reboot the member")
    parser.add_argument("--reboot-offline-timeout-s", type=float, default=20.0)
    parser.add_argument("--member-boot-timeout-s", type=float, default=45.0)
    parser.add_argument("--no-auto-rejoin-s", type=float, default=5.0, help="after manual leave, verify no auto-rejoin for this many seconds")
    parser.add_argument("--initial-drain-s", type=float, default=1.0)
    parser.add_argument("--cmd-timeout-s", type=float, default=8.0)
    parser.add_argument("--state-timeout-s", type=float, default=20.0)
    parser.add_argument("--poll-interval-s", type=float, default=1.0)
    parser.add_argument("--log-dir", default="", help="optional directory to write leader/member raw logs")
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
