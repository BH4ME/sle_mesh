# Version v4.4.65

Date: 2026-06-05

## Scope

`v4.4.65` fixes the runtime pairing admission regression found while testing `v4.4.64` with three boards:

- `COM13`: leader in the latest three-board test, route/self id `241`, MAC suffix `E7F1`
- `COM17`: member/relay candidate, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

## Root Cause

Live serial logs from `logs/live/v4.4.64_three_board_cfg_relay_20260605_072256/run.log` showed:

```text
leader force rescan reason=pairing_window
seek result ... name:1 filter:0 addr:52:**:**:**:e7:e0
[cfg] member-now queued ret=0 leader_suffix=E7F1 leader=241 team=1 channel=17
[sle-tx-fail] type=PACKET dst=241 ret=-4 reason=NOT_READY
```

The member was advertising and trying to join, but the leader still rejected it in `team_leader_should_seek_member()` because the seek filter only accepted bucket-1 candidates unless a failover window already contained that member. During a pairing window, new/unapproved members must be allowed to connect first so they can send HELLO and become pending/approvable.

## What Changed

1. Firmware version bumped to `v4.4.65`.
2. `team_leader_should_seek_member()` now accepts candidate members whenever `pairing_enabled != 0`.
3. The `v4.4.64` relay failover recovery guards remain intact.
4. Local tests now assert the pairing-window seek admission guard is present.
5. Build and flash defaults now expect `v4.4.65`.

## Required Verification

This version is accepted only when logs prove:

1. Remote Ubuntu build links `v4.4.65`.
2. The downloaded `.fwpkg` contains `v4.4.65`.
3. Flashed boards report `fw:"v4.4.65"` from `cfg status`.
4. Leader pairing window accepts non-bucket-1 candidates and sees pending/online members.
5. Three-board relay test reaches online state and exercises relay/member recovery without returning to `on 0 off 1`.

## Command Log Slots

```text
local regression log: pending
build log: pending
firmware package: <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
burn log: pending
three-board test log: pending
COM16 status: pending
```
