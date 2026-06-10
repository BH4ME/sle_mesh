# Version v4.4.131

## Type

Firmware relay-tree stabilization release.

## Firmware Version

The WS63 firmware version is now `v4.4.131`.

## Summary

- Advanced firmware and current repository records from `v4.4.130` to `v4.4.131`.
- Prevented active relay parents from being demoted or migrated while they still have live downstream routes.
- Added a leader-side route-table guard that treats members with active children as direct parents, even if relay policy is being rebalanced.
- Kept the v4.4.130 low-latency heartbeat and failover timing changes.

## Root Cause

- In the nine-board test, member `224` was granted relay and was already forwarding downstream members through `next_hop=224`.
- Relay rebalance later promoted member `154` and demoted `224` because the policy only compared relay count/RSSI and did not check whether `224` was still carrying children.
- After demotion, direct-cap migration reparented `224` under `154`; `224` entered `RESELECT_PARENT`, dropped its leader path, and rejected or failed forwarded child traffic.

## Validation

- Source-level tests assert that active downstream route owners are kept direct and are skipped by the worst-relay demotion selector.
- Python automation syntax/unit tests should be run before flashing.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.131` marker.
- Hardware regression target: 1 leader plus all useful programmed members, observe bootstrap relay election, stable communication, non-relay reboot recovery, relay reboot recovery, and member/relay leave reset recovery.
