#!/usr/bin/env python3
"""WS63 multi-board natural relay enrollment and recovery test."""

from __future__ import annotations

import argparse
import json
import pathlib
import sys
import time
from typing import Iterable

try:
    from automation.ws63.tools import ws63_four_board_relay_test as fb
    from automation.ws63.tools import ws63_link_cycle_test as lc
except ModuleNotFoundError:  # pragma: no cover - direct script execution
    repo_root = pathlib.Path(__file__).resolve().parents[3]
    sys.path.insert(0, str(repo_root))
    from automation.ws63.tools import ws63_four_board_relay_test as fb
    from automation.ws63.tools import ws63_link_cycle_test as lc


def _progress(message: str) -> None:
    print(f"[multi-board] {message}", flush=True)


def _records_line(records: fb.MemberRecords, member_ids: Iterable[int]) -> str:
    parts: list[str] = []
    for member_id in member_ids:
        record = records.get(member_id, {})
        parts.append(
            f"{member_id}:online={record.get('online')} relay={record.get('relay')} "
            f"tier={record.get('tier')} last={record.get('last_seen')}"
        )
    return "; ".join(parts)


def _wait_stable_online(
    leader: lc.Peer,
    peers: list[lc.Peer],
    member_ids: list[int],
    *,
    seconds: float,
    poll_s: float,
) -> fb.MemberRecords:
    _progress(f"stability dwell {seconds:.0f}s: polling members")
    end = time.time() + seconds
    last: fb.MemberRecords = {}
    poll_count = 0
    while time.time() < end:
        records = fb._wait_member_records(
            leader,
            peers,
            expected={member_id: (1, None) for member_id in member_ids},
            timeout_s=10.0,
            poll_s=poll_s,
            note="stability all members online",
        )
        last = records
        poll_count += 1
        relays = fb._relay_ids_from_records(records, member_ids)
        _progress(f"stable poll {poll_count}: relays={relays}; {_records_line(records, member_ids)}")
        time.sleep(min(5.0, max(0.0, end - time.time())))
    return last


def _query_initial_statuses(peers: list[lc.Peer]) -> dict[str, dict[str, object]]:
    statuses: dict[str, dict[str, object]] = {}
    for peer in peers:
        status = fb._query_cfg(peer, peers, window_s=3.0, attempts=5)
        statuses[peer.name] = status
        _progress(
            f"{peer.name} {peer.port}: fw={status.get('fw')} suffix={status.get('selfSuffix')} "
            f"route={status.get('routeId')} role={status.get('runtimeRole')}"
        )
    return statuses


def run(args: argparse.Namespace) -> int:
    peers: list[lc.Peer] = []
    leader_id: int | None = None
    log_dir = pathlib.Path(args.log_dir)
    summary_path = log_dir / "summary.json"
    summary: dict[str, object] = {
        "leader_port": args.leader_port,
        "member_ports": args.member_ports,
        "team_id": args.team_id,
        "channel": args.channel,
        "direct_cap": args.direct_cap,
        "log_dir": str(log_dir),
    }
    try:
        if len(args.member_ports) < 2:
            raise RuntimeError("at least two member ports are required to exercise relay failover")

        log_dir.mkdir(parents=True, exist_ok=True)
        leader = lc._open_peer("leader", args.leader_port, args.baudrate)
        members = [
            lc._open_peer(f"member{idx}", port, args.baudrate)
            for idx, port in enumerate(args.member_ports, start=1)
        ]
        peers = [leader, *members]

        _progress("drain serial boot logs")
        fb._drain_all(peers, args.initial_drain_s)

        statuses = _query_initial_statuses(peers)
        summary["initial_statuses"] = statuses
        leader_suffix = str(statuses["leader"].get("selfSuffix"))
        leader_id = int(statuses["leader"].get("routeId"))
        member_ids = [int(statuses[member.name].get("routeId")) for member in members]
        peer_by_id = {member_id: peer for member_id, peer in zip(member_ids, members)}
        all_route_ids = [leader_id, *member_ids]
        if len(set(all_route_ids)) != len(all_route_ids):
            raise RuntimeError(f"route ids must be unique: {all_route_ids}")
        summary["leader_id"] = leader_id
        summary["member_ids"] = member_ids

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
        summary["leader_runtime_after_config"] = leader_status
        _progress(
            f"leader runtime: id={leader_id} suffix={leader_suffix} direct_cap={runtime_direct_cap} "
            f"relay_budget={leader_status.get('runtimeRelayBudget')}"
        )

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
        for member, member_id in zip(members, member_ids):
            _progress(f"wait natural pending/hello: {member.name} {member.port} id={member_id}")
            fb._wait_leader_sees_member(
                leader,
                peers,
                member_id=member_id,
                leader_id=leader_id,
                timeout_s=args.state_timeout_s,
                poll_s=args.poll_interval_s,
            )

        _progress("pairing stop: let firmware auto-policy decide relay roles")
        fb._send_cfg_and_wait(
            leader,
            peers,
            command="pairing stop",
            pattern=r"pairing stop ret=0",
            timeout_s=args.cmd_timeout_s,
            note="pairing stop",
        )

        records = fb._wait_member_records(
            leader,
            peers,
            expected={member_id: (1, None) for member_id in member_ids},
            timeout_s=args.state_timeout_s,
            poll_s=args.poll_interval_s,
            note="all members online after enrollment",
        )
        for member in members:
            fb._wait_peer_joined(member, peers, timeout_s=args.state_timeout_s, poll_s=args.poll_interval_s)

        records = fb._wait_stable_final_topology(
            leader,
            peers,
            leader_id=leader_id,
            member_ids=member_ids,
            direct_cap=runtime_direct_cap,
            timeout_s=args.route_timeout_s,
            poll_s=args.poll_interval_s,
            log_start=route_log_start,
            stable_polls=args.stable_polls,
            allow_any_converged=True,
        )
        relays = fb._relay_ids_from_records(records, member_ids)
        summary["initial_records"] = records
        summary["initial_relays"] = relays
        _progress(f"enrollment PASS: relays={relays}; {_records_line(records, member_ids)}")
        if not relays:
            raise RuntimeError("no bootstrap relay observed after direct-cap pressure")

        stable_records = _wait_stable_online(
            leader,
            peers,
            member_ids,
            seconds=args.stability_s,
            poll_s=args.poll_interval_s,
        )
        summary["stability_records"] = stable_records

        non_relay_id = fb._prefer_non_relay_member_id(stable_records, member_ids)
        if non_relay_id is None:
            raise RuntimeError(f"no non-relay member available for member reboot: {stable_records}")
        non_relay = peer_by_id[non_relay_id]
        _progress(f"member reboot test: reboot non-relay {non_relay.name} {non_relay.port} id={non_relay_id}")
        member_reboot_start = len(leader.log)
        fb._send_cli_line(non_relay, args.reboot_command)
        fb._wait_pattern(
            non_relay,
            peers,
            pattern=r"\[team-nv\] (load role=0 .*|restore member leader_suffix=)",
            timeout_s=args.boot_timeout_s,
            note="member restore from NV",
        )
        fb._wait_leader_observes_member_reboot_rejoin(
            leader,
            peers,
            member_id=non_relay_id,
            leader_id=leader_id,
            timeout_s=args.state_timeout_s,
            poll_s=args.poll_interval_s,
            log_start=member_reboot_start,
        )
        fb._wait_member_records(
            leader,
            peers,
            expected={non_relay_id: (1, None)},
            timeout_s=args.state_timeout_s,
            poll_s=args.poll_interval_s,
            note="member rejoins after reboot",
        )
        fb._wait_peer_joined(non_relay, peers, timeout_s=args.state_timeout_s, poll_s=args.poll_interval_s)
        post_member_records = fb._wait_stable_final_topology(
            leader,
            peers,
            leader_id=leader_id,
            member_ids=member_ids,
            direct_cap=runtime_direct_cap,
            timeout_s=args.route_timeout_s,
            poll_s=args.poll_interval_s,
            log_start=member_reboot_start,
            stable_polls=args.stable_polls,
            allow_any_converged=True,
        )
        fb._assert_no_route_regressions(peers, leader_id)
        post_member_relays = fb._relay_ids_from_records(post_member_records, member_ids)
        summary["member_reboot"] = {
            "member_id": non_relay_id,
            "port": non_relay.port,
            "records": post_member_records,
            "relays": post_member_relays,
        }
        _progress(f"member reboot PASS: relays={post_member_relays}; member={non_relay_id} rejoined")

        if not post_member_relays:
            raise RuntimeError(f"no relay available for relay reboot after member test: {post_member_records}")
        relay_id = post_member_relays[0]
        relay_peer = peer_by_id[relay_id]
        downstream_ids = [member_id for member_id in member_ids if member_id != relay_id]
        _progress(f"relay reboot test: reboot relay {relay_peer.name} {relay_peer.port} id={relay_id}")
        relay_reboot_start = len(leader.log)
        fb._send_cli_line(relay_peer, args.reboot_command)
        lc._wait_leader_offline_event(
            leader,
            peers,
            member_id=relay_id,
            timeout_s=args.offline_timeout_s,
            note="leader offline after relay reboot",
            log_start=relay_reboot_start,
        )
        child_relay_records = fb._wait_any_child_relay(
            leader,
            peers,
            child_ids=downstream_ids,
            timeout_s=args.failover_timeout_s,
            poll_s=args.poll_interval_s,
        )
        elected = [member_id for member_id in downstream_ids if child_relay_records.get(member_id, {}).get("relay") == 1]
        _progress(f"relay failover/bootstrap observed: relay elected/retained {elected}")

        fb._wait_pattern(
            relay_peer,
            peers,
            pattern=r"\[team-nv\] (load role=0 .*|restore member leader_suffix=)",
            timeout_s=args.boot_timeout_s,
            note="relay restore from NV",
        )
        final_records = fb._wait_stable_final_topology(
            leader,
            peers,
            leader_id=leader_id,
            member_ids=member_ids,
            direct_cap=runtime_direct_cap,
            timeout_s=args.route_timeout_s,
            poll_s=args.poll_interval_s,
            log_start=relay_reboot_start,
            stable_polls=args.stable_polls,
            allow_any_converged=True,
        )
        fb._assert_no_route_regressions(peers, leader_id)
        final_relays = fb._relay_ids_from_records(final_records, member_ids)
        policy = fb._summarize_policy(final_records, relay_id, downstream_ids)
        summary["relay_reboot"] = {
            "relay_id": relay_id,
            "port": relay_peer.port,
            "elected_or_retained": elected,
            "final_records": final_records,
            "final_relays": final_relays,
            "policy": policy,
        }
        summary["result"] = "PASS"
        _progress(f"relay recovery PASS: {policy}; final_relays={final_relays}")
        _progress("PASS")
        return 0
    except Exception as exc:  # noqa: BLE001
        _progress(f"FAIL: {exc}")
        summary["result"] = "FAIL"
        summary["error"] = str(exc)
        if leader_id is not None:
            regressions = fb._find_route_regression_events(peers, leader_id)
            if regressions:
                _progress("route regression evidence: " + "; ".join(regressions[:5]))
                summary["route_regressions"] = regressions[:20]
        return 1
    finally:
        if peers:
            try:
                fb._dump_logs(peers, log_dir)
                _progress(f"logs saved: {log_dir}")
            except Exception as dump_exc:  # noqa: BLE001
                _progress(f"WARN: failed to dump logs: {dump_exc}")
        try:
            log_dir.mkdir(parents=True, exist_ok=True)
            summary_path.write_text(
                json.dumps(summary, indent=2, ensure_ascii=False, default=str),
                encoding="utf-8",
            )
            _progress(f"summary saved: {summary_path}")
        except Exception as summary_exc:  # noqa: BLE001
            _progress(f"WARN: failed to write summary: {summary_exc}")
        for peer in peers:
            try:
                peer.ser.close()
            except Exception:  # noqa: BLE001
                pass


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(description="WS63 live multi-board natural relay recovery test")
    parser.add_argument("--leader-port", required=True)
    parser.add_argument("--member-ports", nargs="+", required=True)
    parser.add_argument("--baudrate", type=int, default=115200)
    parser.add_argument("--team-id", type=int, default=7)
    parser.add_argument("--channel", type=int, default=33)
    parser.add_argument("--direct-cap", type=int, default=4)
    parser.add_argument("--skip-direct-config", action="store_true")
    parser.add_argument("--reboot-command", default="cfg reboot")
    parser.add_argument("--no-clean-start", action="store_true")
    parser.add_argument("--initial-drain-s", type=float, default=2.0)
    parser.add_argument("--cmd-timeout-s", type=float, default=20.0)
    parser.add_argument("--boot-timeout-s", type=float, default=85.0)
    parser.add_argument("--state-timeout-s", type=float, default=100.0)
    parser.add_argument("--route-timeout-s", type=float, default=140.0)
    parser.add_argument("--offline-timeout-s", type=float, default=45.0)
    parser.add_argument("--failover-timeout-s", type=float, default=120.0)
    parser.add_argument("--stability-s", type=float, default=45.0)
    parser.add_argument("--stable-polls", type=int, default=4)
    parser.add_argument("--poll-interval-s", type=float, default=1.0)
    parser.add_argument("--log-dir", required=True)
    return parser


def main() -> int:
    return run(build_parser().parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
