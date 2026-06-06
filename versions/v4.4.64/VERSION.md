# Version v4.4.64

Date: 2026-06-05

## Scope

`v4.4.64` fixes the four-board relay failover failure found in `v4.4.63`.

Target topology:

- `COM16`: leader, route/self id `154`, MAC suffix `279A`
- `COM13`: original relay candidate, route/self id `241`, MAC suffix `E7F1`
- `COM17`: child member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

Leader config:

```text
cfg leader now 1 17
cfg direct 1
```

## Root Cause

Live `v4.4.63` logs proved the leader did promote a child relay after `COM13/241` went offline, but the next rebalance pass demoted it:

```text
[team] relay offline event member=241 trigger immediate rebalance
[team] relay failover begin lost=241 watched=1 grace=30
[team] relay set member=86 allow=1 notify=1 reason=auto-promote ret=0
[team] relay rebalance online=2 relay=1 target=1 changed=1
[state] member timeout deferred after relay loss
[team] relay set member=86 allow=0 notify=1 reason=auto-demote ret=0
[team] relay rebalance online=1 relay=0 target=0 changed=1
```

The normal relay target calculation uses current online member count. During relay loss, downstream children can temporarily disappear from the leader view, so `online_count <= direct_cap` made the target drop to `0`, even though the failover window still needed one relay to rebuild the downstream path.

## What Changed

1. Firmware version bumped to `v4.4.64`.
2. Leader rebalance now calls `team_leader_relay_target_with_failover(...)`.
3. During an active relay failover window, if a temporary relay already exists and the normal target falls to `0`, leader holds target at `1`.
4. The firmware logs `relay failover holding relay target` so serial logs can prove the guard executed.
5. Build and flash defaults now expect `v4.4.64`.

## Required Live Verification

This version is accepted only when serial logs prove:

1. All four boards report `fw:"v4.4.64"` from `cfg status`.
2. `COM16` is leader with `runtimeDirectCap=1`.
3. `COM13`, `COM17`, and `COM18` are members of leader suffix `279A`.
4. Pairing/enrollment yields `241` as relay and `224/86` as normal members.
5. Route metrics settle to `active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1`.
6. Rebooting child1 produces leader timeout/offline evidence for `224`, then `224 online=1` and child1 `joined=1`.
7. Rebooting relay produces leader relay-offline evidence, child relay election evidence, and no auto-demote of the temporary child relay during the failover window.
8. All three members are online after original relay recovery.
9. Final log states whether original relay regained relay role or returned as a normal member.

## Command Log Slots

```text
build log: pending
firmware package: <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
burn log: pending
four-board test log: pending
final policy: pending
```
