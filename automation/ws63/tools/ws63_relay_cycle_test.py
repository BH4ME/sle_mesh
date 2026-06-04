#!/usr/bin/env python3
"""WS63 three-board relay failover serial test.

Validates the live relay requirement with three boards:
leader + relay member + child member. The relay is rebooted to simulate
signal loss, not manual leave. The child must reselect an upstream parent and
continue/recover communication with the leader.
"""

from __future__ import annotations

import argparse
import pathlib
import re
import sys
import time
from typing import Iterable, Optional

try:
    from automation.ws63.tools import ws63_link_cycle_test as lc
except ModuleNotFoundError:  # pragma: no cover - direct script execution
    repo_root = pathlib.Path(__file__).resolve().parents[3]
    sys.path.insert(0, str(repo_root))
    from automation.ws63.tools import ws63_link_cycle_test as lc


MemberRecord = dict[str, int]


def _parse_member_records(text: str) -> dict[int, MemberRecord]:
    records: dict[int, MemberRecord] = {}
    for line in text.splitlines():
        match = re.search(
            r"member=(\d+).*?\bonline=(\d+).*?\brelay=(\d+).*?\btier=(\d+).*?\bmax_down=(\d+).*?\blast_seen=(\d+)",
            line,
        )
        if not match:
            continue
        member_id = int(match.group(1))
        records[member_id] = {
            "online": int(match.group(2)),
            "relay": int(match.group(3)),
            "tier": int(match.group(4)),
            "max_down": int(match.group(5)),
            "last_seen": int(match.group(6)),
        }
    return records


def _query_member_record(
    leader: lc.Peer,
    peers: Iterable[lc.Peer],
    member_id: int,
    query_window_s: float,
) -> Optional[MemberRecord]:
    leader.send_line("members")
    end = time.time() + query_window_s
    buf = ""
    while time.time() < end:
        for peer in peers:
            chunk = lc._read_once(peer)
            if peer is leader and chunk:
                buf += chunk
        time.sleep(0.02)
    return _parse_member_records(buf).get(member_id)


def _wait_member_record(
    leader: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    member_id: int,
    expect_online: Optional[int],
    expect_relay: Optional[int],
    timeout_s: float,
    poll_s: float,
    note: str,
) -> MemberRecord:
    end = time.time() + timeout_s
    last: Optional[MemberRecord] = None
    while time.time() < end:
        record = _query_member_record(leader, peers, member_id, query_window_s=min(0.6, poll_s))
        if record is not None:
            last = record
            online_ok = expect_online is None or record["online"] == expect_online
            relay_ok = expect_relay is None or record["relay"] == expect_relay
            if online_ok and relay_ok:
                return record
        time.sleep(max(0.1, poll_s - 0.6))
    raise RuntimeError(f"timeout waiting for {note}: member={member_id} last={last}")


def _query_joined(peer: lc.Peer, peers: Iterable[lc.Peer], query_window_s: float) -> Optional[int]:
    peer.send_line("state")
    end = time.time() + query_window_s
    buf = ""
    while time.time() < end:
        for item in peers:
            chunk = lc._read_once(item)
            if item is peer and chunk:
                buf += chunk
        time.sleep(0.02)

    joined: Optional[int] = None
    for match in re.finditer(r"\bjoined=(\d+)\b", buf):
        joined = int(match.group(1))
    return joined


def _wait_peer_joined(
    peer: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    timeout_s: float,
    poll_s: float,
    note: str,
) -> None:
    end = time.time() + timeout_s
    while time.time() < end:
        joined = _query_joined(peer, peers, query_window_s=min(0.6, poll_s))
        if joined == 1:
            return
        time.sleep(max(0.1, poll_s - 0.6))
    raise RuntimeError(f"timeout waiting for {note}: joined=1")


def _wait_log_pattern(
    target: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    pattern: str,
    timeout_s: float,
    note: str,
    log_start: int = 0,
) -> None:
    rx = re.compile(pattern)
    end = time.time() + timeout_s
    while time.time() < end:
        if rx.search("".join(target.log[log_start:])):
            return
        for peer in peers:
            lc._read_once(peer)
        time.sleep(0.02)
    raise RuntimeError(f"timeout waiting for {note}: pattern={pattern}")


def _collect_leader_visibility(
    leader: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    member_id: int,
    leader_id: int,
    timeout_s: float,
    poll_s: float,
) -> None:
    end = time.time() + timeout_s
    start = len(leader.log)
    pattern = re.compile(rf"(pending member={member_id}\b|HELLO {member_id}->{leader_id}\b)")
    while time.time() < end:
        if pattern.search("".join(leader.log[start:])):
            return
        record = _query_member_record(leader, peers, member_id, query_window_s=min(0.4, poll_s))
        if record is not None:
            return
        leader.send_line("pairing pending")
        wait_end = time.time() + min(0.4, poll_s)
        while time.time() < wait_end:
            for peer in peers:
                lc._read_once(peer)
            time.sleep(0.02)
    raise RuntimeError(f"leader did not see member={member_id} pending/hello before approval")


def _configure_member(
    leader: lc.Peer,
    member: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    member_id: int,
    leader_suffix: int,
    leader_id: int,
    team_id: int,
    channel: int,
    approve_relay: int,
    bootstrap_roles: bool,
    cmd_timeout_s: float,
    state_timeout_s: float,
    poll_interval_s: float,
    note: str,
) -> None:
    if bootstrap_roles:
        member.send_line(f"role member {leader_suffix:04X}")
        lc._wait_regex(member, peers, r"role member leader_suffix=[0-9A-Fa-f]{4} .*ret=0", cmd_timeout_s,
                       f"{note} role member")
        lc._wait_role(member, peers, expected_role=0, timeout_s=state_timeout_s, poll_s=poll_interval_s)
    else:
        member.send_line(f"join {team_id} {leader_id} {channel}")
        lc._wait_regex(member, peers, r"join team=.* ret=0", cmd_timeout_s, f"{note} join")

    _collect_leader_visibility(
        leader,
        peers,
        member_id=member_id,
        leader_id=leader_id,
        timeout_s=state_timeout_s,
        poll_s=poll_interval_s,
    )
    leader.send_line(f"pairing approve {member_id} {'relay' if approve_relay else 'norelay'}")
    lc._wait_regex(
        leader,
        peers,
        rf"pairing approve member={member_id} relay={approve_relay} ret=0",
        cmd_timeout_s,
        f"{note} approve",
    )
    _wait_member_record(
        leader,
        peers,
        member_id=member_id,
        expect_online=1,
        expect_relay=approve_relay,
        timeout_s=state_timeout_s,
        poll_s=poll_interval_s,
        note=f"{note} leader member table",
    )
    _wait_peer_joined(member, peers, timeout_s=state_timeout_s, poll_s=poll_interval_s, note=f"{note} joined")


def _detect_route_id(
    peer: lc.Peer,
    peers: Iterable[lc.Peer],
    *,
    provided_id: int,
    timeout_s: float,
    note: str,
) -> int:
    if provided_id > 0:
        return provided_id
    suffix = lc._detect_suffix(peer, peers, "", timeout_s, note)
    return lc._route_id_from_suffix(suffix)


def _dump_logs(peers: Iterable[lc.Peer], out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for peer in peers:
        path = out_dir / f"{peer.name}_{pathlib.Path(peer.port).name}.log"
        with path.open("w", encoding="utf-8", errors="ignore") as handle:
            for item in peer.log:
                handle.write(item)


def run(args: argparse.Namespace) -> int:
    peers: list[lc.Peer] = []
    try:
        leader = lc._open_peer("leader", args.leader_port, args.baudrate)
        relay = lc._open_peer("relay", args.relay_port, args.baudrate)
        child = lc._open_peer("child", args.child_port, args.baudrate)
        peers = [leader, relay, child]

        lc._drain(peers, args.initial_drain_s)

        leader_suffix = lc._detect_suffix(
            leader,
            peers,
            args.leader_suffix,
            args.bootstrap_timeout_s,
            "leader wifi status",
        )
        leader_id = args.leader_id if args.leader_id > 0 else lc._route_id_from_suffix(leader_suffix)
        relay_id = _detect_route_id(relay, peers, provided_id=args.relay_id,
                                    timeout_s=args.bootstrap_timeout_s, note="relay wifi status")
        child_id = _detect_route_id(child, peers, provided_id=args.child_id,
                                    timeout_s=args.bootstrap_timeout_s, note="child wifi status")
        if len({leader_id, relay_id, child_id}) != 3:
            raise RuntimeError(f"route ids must be unique: leader={leader_id} relay={relay_id} child={child_id}")

        if args.bootstrap_roles:
            leader.send_line("role leader")
            lc._wait_regex(leader, peers, r"role leader ret=0", args.bootstrap_timeout_s, "set leader role")
            lc._wait_role(leader, peers, expected_role=1, timeout_s=args.bootstrap_timeout_s,
                          poll_s=args.poll_interval_s)

        leader.send_line("pairing start")
        lc._wait_regex(leader, peers, r"pairing start ret=0", args.cmd_timeout_s, "pairing start")

        _configure_member(
            leader,
            relay,
            peers,
            member_id=relay_id,
            leader_suffix=leader_suffix,
            leader_id=leader_id,
            team_id=args.team_id,
            channel=args.channel,
            approve_relay=1,
            bootstrap_roles=args.bootstrap_roles,
            cmd_timeout_s=args.cmd_timeout_s,
            state_timeout_s=args.state_timeout_s,
            poll_interval_s=args.poll_interval_s,
            note="relay",
        )

        child_parent_log_start = len(child.log)
        _configure_member(
            leader,
            child,
            peers,
            member_id=child_id,
            leader_suffix=leader_suffix,
            leader_id=leader_id,
            team_id=args.team_id,
            channel=args.channel,
            approve_relay=0,
            bootstrap_roles=args.bootstrap_roles,
            cmd_timeout_s=args.cmd_timeout_s,
            state_timeout_s=args.state_timeout_s,
            poll_interval_s=args.poll_interval_s,
            note="child",
        )

        if args.require_child_parent_relay:
            _wait_log_pattern(
                child,
                peers,
                pattern=rf"upstream parent={relay_id}\b.*state=",
                timeout_s=args.parent_timeout_s,
                note="child initially uses relay as upstream parent",
                log_start=child_parent_log_start,
            )

        leader.send_line("pairing stop")
        lc._wait_regex(leader, peers, r"pairing stop ret=0", args.cmd_timeout_s, "pairing stop")

        leader_offline_log_start = len(leader.log)
        child_failover_log_start = len(child.log)
        relay.send_line(args.relay_reboot_command)
        lc._wait_leader_offline_event(
            leader,
            peers,
            member_id=relay_id,
            timeout_s=args.relay_offline_timeout_s,
            note="leader offline after relay reboot",
            log_start=leader_offline_log_start,
        )
        _wait_log_pattern(
            child,
            peers,
            pattern=rf"(upstream parent reselect parent={relay_id}\b|upstream parent=(?!{relay_id}\b)\d+\b.*state=|joined member={child_id}\b)",
            timeout_s=args.failover_timeout_s,
            note="child reselects upstream parent after relay signal loss",
            log_start=child_failover_log_start,
        )
        _wait_peer_joined(child, peers, timeout_s=args.failover_timeout_s,
                          poll_s=args.poll_interval_s, note="child remains/rejoins team after relay loss")
        _wait_member_record(
            leader,
            peers,
            member_id=child_id,
            expect_online=1,
            expect_relay=None,
            timeout_s=args.failover_timeout_s,
            poll_s=args.poll_interval_s,
            note="leader sees child online after relay loss",
        )

        if not args.skip_pos_report:
            child.send_line(f"pos {leader_id} 0 0 0 0 88 1 0")
            _wait_log_pattern(
                leader,
                peers,
                pattern=rf"POS_REPORT {child_id}->{leader_id}\b",
                timeout_s=args.failover_timeout_s,
                note="leader receives child POS_REPORT after relay loss",
                log_start=leader_offline_log_start,
            )

        lc._wait_regex(
            relay,
            peers,
            r"\[team-nv\] (load role=0 .*|restore member leader_suffix=)",
            args.relay_boot_timeout_s,
            "relay restore from NV after reboot",
        )
        _wait_member_record(
            leader,
            peers,
            member_id=relay_id,
            expect_online=1,
            expect_relay=None,
            timeout_s=args.state_timeout_s,
            poll_s=args.poll_interval_s,
            note="relay rejoins after reboot",
        )

        print(
            "[relay-cycle] PASS: relay reboot/loss recovered child route "
            f"leader={leader_id} relay={relay_id} child={child_id}"
        )
        return 0
    except Exception as exc:  # noqa: BLE001
        print(f"[relay-cycle] FAIL: {exc}")
        return 1
    finally:
        if args.log_dir:
            _dump_logs(peers, pathlib.Path(args.log_dir))
        for peer in peers:
            try:
                peer.ser.close()
            except Exception:  # noqa: BLE001
                pass


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="WS63 live three-board relay failover test")
    parser.add_argument("--leader-port", required=True, help="leader serial port")
    parser.add_argument("--relay-port", required=True, help="relay member serial port")
    parser.add_argument("--child-port", required=True, help="child member serial port")
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--bootstrap-roles", action="store_true", help="configure leader/member roles before test")
    parser.add_argument("--leader-suffix", default="", help="optional leader MAC suffix hex")
    parser.add_argument("--team-id", type=int, default=1)
    parser.add_argument("--leader-id", type=int, default=-1)
    parser.add_argument("--relay-id", type=int, default=-1)
    parser.add_argument("--child-id", type=int, default=-1)
    parser.add_argument("--channel", type=int, default=17)
    parser.add_argument("--relay-reboot-command", default="reboot")
    parser.add_argument("--initial-drain-s", type=float, default=1.0)
    parser.add_argument("--bootstrap-timeout-s", type=float, default=20.0)
    parser.add_argument("--cmd-timeout-s", type=float, default=8.0)
    parser.add_argument("--state-timeout-s", type=float, default=30.0)
    parser.add_argument("--parent-timeout-s", type=float, default=30.0)
    parser.add_argument("--relay-offline-timeout-s", type=float, default=30.0)
    parser.add_argument("--relay-boot-timeout-s", type=float, default=60.0)
    parser.add_argument("--failover-timeout-s", type=float, default=60.0)
    parser.add_argument("--poll-interval-s", type=float, default=1.0)
    parser.add_argument("--no-require-child-parent-relay", dest="require_child_parent_relay", action="store_false")
    parser.add_argument("--skip-pos-report", action="store_true")
    parser.add_argument("--log-dir", default="")
    parser.set_defaults(require_child_parent_relay=True)
    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    return run(args)


if __name__ == "__main__":
    raise SystemExit(main())
