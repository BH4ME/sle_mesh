# Version v4.4.130

## Type

Firmware low-latency relay recovery release.

## Firmware Version

The WS63 firmware version is now `v4.4.130`.

## Summary

- Advanced firmware and current repository records from `v4.4.129` to `v4.4.130`.
- Shortened heartbeat loss detection from 4 s to 3 s; member parent timeout is now 1 s.
- Shortened route metrics, route-hint retry, relay rebalance, member rescan, and relay config retry intervals.
- Shortened relay failover grace from 30 s to 6 s.
- Fixed pairing-window natural enrollment by making `sle_uart_client_force_rescan()` stop an active seek and restart scanning.
- Fixed relay failover recovery by clearing stale routes through the lost relay instead of preserving dead next-hop routes.
- Made relay failover begin idempotent so duplicate offline callbacks do not restart the grace window.
- Changed failover direct-cap handling so only members whose new relay parent is not ready are deferred.
- Drops child SLE connections when a member is demoted from relay, forcing downstream members to reselect a valid parent.

## Root Cause

- Natural pairing could deadlock because the leader entered pairing while the SLE client still reported `seek active conn:0`; repeated rescans skipped and bucket-policy rejects continued before the pairing filter could accept all candidates.
- Relay recovery could stall because failover preserved routes through the lost relay, deferred member timeout for the full grace window, and globally deferred direct-cap migration. This left children talking to a stale or demoted parent and produced `NO_ROUTE` / relay-forward rejection loops.

## Validation

- Source-level tests assert the `v4.4.130` firmware marker, shortened recovery constants, force-rescan seek restart, pairing scan restart hook, stale-route clearing, and relay-demote child cleanup.
- Python automation syntax/unit tests should be run before flashing.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.130` marker.
- Hardware regression target: 1 leader plus all useful programmed members, observe bootstrap relay election, stable communication, non-relay reboot recovery, relay reboot recovery, and member/relay leave reset recovery.
