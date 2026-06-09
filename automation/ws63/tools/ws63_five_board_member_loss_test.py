#!/usr/bin/env python3
"""WS63 five-board leader/member enrollment and member-loss test."""

from __future__ import annotations

import argparse
import pathlib
import sys
import time
from typing import Optional

try:
    from automation.ws63.tools import ws63_four_board_relay_test as fb
    from automation.ws63.tools import ws63_link_cycle_test as lc
except ModuleNotFoundError:  # pragma: no cover - direct script execution
    repo_root = pathlib.Path(__file__).resolve().parents[3]
    sys.path.insert(0, str(repo_root))
    from automation.ws63.tools import ws63_four_board_relay_test as fb
    from automation.ws63.tools import ws63_link_cycle_test as lc


def _progress(message: str) -> None:
    print(f"[five-board] {message}", flush=True)


def _dump_logs(peers: list[lc.Peer], out_dir: pathlib.Path) -> None:
    out_dir.mkdir(parents=True, exist_ok=True)
    for peer in peers:
        path = out_dir / f"{peer.name}_{peer.port}.log"
        with path.open("w", encoding="utf-8", errors="ignore") as handle:
            for item in peer.log:
                handle.write(item)


def _select_disconnect_member(
    members: list[lc.Peer],
    member_ids: dict[str, int],
    requested_port: str,
) -> lc.Peer:
    if requested_port:
        wanted = requested_port.upper()
        for member in members:
            if member.port.upper() == wanted:
                return member
        raise RuntimeError(f"disconnect port {requested_port} is not in member ports")
    return members[0]


def _records_summary(records: fb.MemberRecords, member_ids: list[int]) -> str:
    parts: list[str] = []
    for member_id in member_ids:
        record = records.get(member_id)
        if record is None:
            parts.append(f"{member_id}:missing")
            continue
        parts.append(
            f"{member_id}:online={record.get('online')} relay={record.get('relay')} "
            f"tier={record.get('tier')} last_seen={record.get('last_seen')}"
        )
    return "; ".join(parts)


def run(args: argparse.Namespace) -> int:
    peers: list[lc.Peer] = []
    try:
        if len(args.member_ports) < 1:
            raise RuntimeError("at least one --member-ports value is required")

        leader = lc._open_peer("leader", args.leader_port, args.baudrate)
        members = [
            lc._open_peer(f"member{idx}", port, args.baudrate)
            for idx, port in enumerate(args.member_ports, start=1)
        ]
        peers = [leader, *members]

        _progress("drain serial boot logs")
        fb._drain_all(peers, args.initial_drain_s)

        statuses = fb._assert_fw(peers, peers, args.expected_fw)
        leader_suffix = str(statuses["leader"]["selfSuffix"])
        leader_id = int(statuses["leader"]["routeId"])
        member_ids = {member.name: int(statuses[member.name]["routeId"]) for member in members}
        all_route_ids = [leader_id, *member_ids.values()]
        if len(set(all_route_ids)) != len(all_route_ids):
            raise RuntimeError(f"route ids must be unique: {all_route_ids}")

        if not args.no_clean_start:
            fb._clean_start_saved_config(
                peers,
                timeout_s=args.cmd_timeout_s,
                boot_timeout_s=args.boot_timeout_s,
                reboot_command=args.reboot_command,
            )

        leader_status = fb._configure_roles(
            leader,
            members,
            peers,
            leader_suffix=leader_suffix,
            team_id=args.team_id,
            channel=args.channel,
            direct_cap=args.direct_cap,
            skip_direct_config=args.skip_direct_config,
            timeout_s=args.cmd_timeout_s,
            boot_timeout_s=args.boot_timeout_s,
        )
        runtime_direct_cap = int(leader_status.get("runtimeDirectCap") or args.direct_cap)
        _progress(
            f"leader runtime: id={leader_id} suffix={leader_suffix} "
            f"direct_cap={runtime_direct_cap} relay_budget={leader_status.get('runtimeRelayBudget')}"
        )

        relay_ports = {port.upper() for port in args.relay_member_ports}
        expected: dict[int, tuple[Optional[int], Optional[int]]] = {}

        route_log_start = len(leader.log)
        _progress("pairing start")
        fb._send_cfg_and_wait(
            leader,
            peers,
            command="pairing start",
            pattern=r"pairing start ret=0",
            timeout_s=args.cmd_timeout_s,
            note="pairing start",
        )
        for member in members:
            member_id = member_ids[member.name]
            relay = 1 if member.port.upper() in relay_ports else 0
            fb._approve_member(
                leader,
                member,
                peers,
                member_id=member_id,
                leader_id=leader_id,
                relay=relay,
                timeout_s=args.state_timeout_s,
                poll_s=args.poll_interval_s,
            )
            expected[member_id] = (1, relay)

        _progress("pairing stop")
        fb._send_cfg_and_wait(
            leader,
            peers,
            command="pairing stop",
            pattern=r"pairing stop ret=0",
            timeout_s=args.cmd_timeout_s,
            note="pairing stop",
        )

        initial_records = fb._wait_member_records(
            leader,
            peers,
            expected=expected,
            timeout_s=args.state_timeout_s,
            poll_s=args.poll_interval_s,
            note="all members online after enrollment",
        )
        for member in members:
            fb._wait_peer_joined(
                member,
                peers,
                timeout_s=args.state_timeout_s,
                poll_s=args.poll_interval_s,
            )
        if args.require_route_converged:
            initial_records = fb._wait_stable_final_topology(
                leader,
                peers,
                leader_id=leader_id,
                member_ids=list(member_ids.values()),
                direct_cap=runtime_direct_cap,
                timeout_s=args.topology_timeout_s,
                poll_s=args.poll_interval_s,
                log_start=route_log_start,
                allow_any_converged=False,
            )
            _progress("route topology PASS")
        _progress(f"enrollment PASS: {_records_summary(initial_records, list(member_ids.values()))}")

        disconnect_member = _select_disconnect_member(members, member_ids, args.disconnect_member_port)
        disconnect_member_id = member_ids[disconnect_member.name]
        _progress(
            f"simulate disconnect: {args.disconnect_method} "
            f"{disconnect_member.name} {disconnect_member.port} id={disconnect_member_id}"
        )
        leader_offline_log_start = len(leader.log)
        if args.disconnect_method == "leave":
            fb._send_cfg_and_wait(
                disconnect_member,
                peers,
                command="leave",
                pattern=r"leave ret=0",
                timeout_s=args.cmd_timeout_s,
                note=f"{disconnect_member.name} leave",
            )
        else:
            fb._send_cli_line(disconnect_member, args.reboot_command)

        lc._wait_leader_offline_event(
            leader,
            peers,
            member_id=disconnect_member_id,
            timeout_s=args.offline_timeout_s,
            note=f"leader offline after {disconnect_member.name} {args.disconnect_method}",
            log_start=leader_offline_log_start,
        )
        _progress(f"leader offline PASS: member={disconnect_member_id}")

        if args.disconnect_method == "reboot" and not args.skip_rejoin_check:
            fb._wait_leader_observes_member_reboot_rejoin(
                leader,
                peers,
                member_id=disconnect_member_id,
                leader_id=leader_id,
                timeout_s=args.rejoin_timeout_s,
                poll_s=args.poll_interval_s,
                log_start=leader_offline_log_start,
            )
            _progress(f"rejoin PASS: member={disconnect_member_id}")

        final_records = fb._query_records_once(leader, peers, window_s=1.0)
        _progress(f"final members: {_records_summary(final_records, list(member_ids.values()))}")
        _progress("PASS")
        return 0
    except Exception as exc:  # noqa: BLE001
        _progress(f"FAIL: {exc}")
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
    parser = argparse.ArgumentParser(description="WS63 live five-board member loss test")
    parser.add_argument("--leader-port", required=True)
    parser.add_argument("--member-ports", nargs="+", required=True)
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--expected-fw", default="v4.4.124")
    parser.add_argument("--team-id", type=int, default=7)
    parser.add_argument("--channel", type=int, default=33)
    parser.add_argument("--direct-cap", type=int, default=4)
    parser.add_argument("--skip-direct-config", action="store_true")
    parser.add_argument("--relay-member-ports", nargs="*", default=[])
    parser.add_argument("--require-route-converged", action="store_true")
    parser.add_argument("--disconnect-member-port", default="")
    parser.add_argument("--disconnect-method", choices=("reboot", "leave"), default="reboot")
    parser.add_argument("--reboot-command", default="cfg reboot")
    parser.add_argument("--no-clean-start", action="store_true")
    parser.add_argument("--skip-rejoin-check", action="store_true")
    parser.add_argument("--initial-drain-s", type=float, default=1.0)
    parser.add_argument("--cmd-timeout-s", type=float, default=10.0)
    parser.add_argument("--boot-timeout-s", type=float, default=35.0)
    parser.add_argument("--state-timeout-s", type=float, default=45.0)
    parser.add_argument("--topology-timeout-s", type=float, default=60.0)
    parser.add_argument("--offline-timeout-s", type=float, default=30.0)
    parser.add_argument("--rejoin-timeout-s", type=float, default=60.0)
    parser.add_argument("--poll-interval-s", type=float, default=1.0)
    parser.add_argument("--log-dir", default="")
    return parser


def main() -> int:
    parser = build_parser()
    return run(parser.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
