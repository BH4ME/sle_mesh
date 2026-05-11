#!/usr/bin/env python3
"""WS63 serial link-cycle test.

Automates a leader/member smoke cycle:
1) pairing start + member join + pairing stop
2) verify member online on leader
3) member leave
4) verify member offline on leader
5) join again and verify online
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
    ser = serial.Serial(port, baudrate=baudrate, timeout=0.1)
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


def _bootstrap_roles(
    leader: Peer,
    member: Peer,
    peers: Iterable[Peer],
    *,
    leader_suffix_hex: str,
    bootstrap_timeout_s: float,
    poll_interval_s: float,
) -> int:
    if leader_suffix_hex:
        leader_suffix = int(leader_suffix_hex, 16) & 0xFFFF
    else:
        leader.send_line("wifi")
        leader_suffix = _wait_and_extract_suffix(
            leader,
            peers,
            timeout_s=bootstrap_timeout_s,
            note="leader wifi status",
        )
    leader_id = _route_id_from_suffix(leader_suffix)

    leader.send_line("role leader")
    _wait_regex(leader, peers, r"role leader ret=0", bootstrap_timeout_s, "set leader role")

    member.send_line(f"role member {leader_suffix:04X}")
    _wait_regex(member, peers, r"role member leader_suffix=[0-9A-Fa-f]{4} ret=0", bootstrap_timeout_s, "set member role")

    _wait_role(leader, peers, expected_role=1, timeout_s=bootstrap_timeout_s, poll_s=poll_interval_s)
    _wait_role(member, peers, expected_role=0, timeout_s=bootstrap_timeout_s, poll_s=poll_interval_s)
    return leader_id


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

        resolved_leader_id = args.leader_id
        if args.bootstrap_roles:
            resolved_leader_id = _bootstrap_roles(
                leader,
                member,
                peers,
                leader_suffix_hex=args.leader_suffix,
                bootstrap_timeout_s=args.bootstrap_timeout_s,
                poll_interval_s=args.poll_interval_s,
            )
            if args.leader_id > 0 and args.leader_id != resolved_leader_id:
                print(
                    "[link-cycle] WARN: provided --leader-id=%d differs from derived leader_id=%d, use derived id"
                    % (args.leader_id, resolved_leader_id)
                )

        if resolved_leader_id <= 0:
            resolved_leader_id = 1

        leader.send_line("pairing start")
        _wait_regex(leader, peers, r"pairing start ret=0", args.cmd_timeout_s, "pairing start")

        member.send_line(f"join {args.team_id} {resolved_leader_id} {args.channel}")
        _wait_regex(member, peers, r"join team=.* ret=0", args.cmd_timeout_s, "member join")

        leader.send_line("pairing stop")
        _wait_regex(leader, peers, r"pairing stop ret=0", args.cmd_timeout_s, "pairing stop")

        _wait_member_state(leader, peers, args.member_id, 1, args.state_timeout_s, args.poll_interval_s)

        member.send_line("leave")
        _wait_regex(member, peers, r"leave ret=0", args.cmd_timeout_s, "member leave")
        _wait_member_state(leader, peers, args.member_id, 0, args.state_timeout_s, args.poll_interval_s)

        leader.send_line("pairing start")
        _wait_regex(leader, peers, r"pairing start ret=0", args.cmd_timeout_s, "pairing start(rejoin)")

        member.send_line(f"join {args.team_id} {resolved_leader_id} {args.channel}")
        _wait_regex(member, peers, r"join team=.* ret=0", args.cmd_timeout_s, "member rejoin")

        leader.send_line("pairing stop")
        _wait_regex(leader, peers, r"pairing stop ret=0", args.cmd_timeout_s, "pairing stop(rejoin)")
        _wait_member_state(leader, peers, args.member_id, 1, args.state_timeout_s, args.poll_interval_s)

        print("[link-cycle] PASS: connect -> leave -> reconnect")
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
