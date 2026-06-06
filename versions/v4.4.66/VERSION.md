# Version v4.4.66

Date: 2026-06-05

## Scope

`v4.4.66` fixes the relay/member reboot recovery regression found in the `v4.4.65` three-board test.

Test topology:

- `COM13`: leader, route/self id `241`, MAC suffix `E7F1`
- `COM17`: relay candidate/member, route/self id `224`, MAC suffix `E7E0`
- `COM18`: child member, route/self id `86`, MAC suffix `5556`

## Root Cause

`v4.4.65` fixed open pairing admission, but the relay reboot path still failed after pairing was closed. The leader serial log in `logs/live/v4.4.65_three_board_cfg_relay_20260605_081106` showed repeated advertisements from member `224` / `E7E0` with `filter:0`. The member was alive and advertising, but the leader refused to seek it because `team_leader_should_seek_member()` only accepted bucket-1 candidates or failover-window watched members after pairing closed.

That blocked a previously approved/known member from reconnecting after its own reboot, so it could not send HELLO and could not refresh liveness.

## What Changed

1. Firmware version bumped to `v4.4.66`.
2. Leader seek admission now accepts a candidate when it already has a member record.
3. Leader seek admission also accepts candidates present in the configured allowlist.
4. The existing pairing-window admission guard from `v4.4.65` remains intact.
5. Tests now assert the known-member and allowlist seek-admission guards are present.

## Required Verification

This version is accepted only when logs prove:

1. Remote Ubuntu build links `v4.4.66`.
2. The downloaded `.fwpkg` contains `v4.4.66`.
3. Flashed boards report `fw:"v4.4.66"` from `cfg status`.
4. After relay/member reboot, leader no longer filters the known member as `filter:0`.
5. Three-board relay test recovers online state instead of returning to `on 0 off 1`.

## Command Log Slots

```text
local regression log: <repo-root>\logs\local\v4.4.66_20260605_082102\local_regression.log
build log: <repo-root>\logs\build\v4.4.66_20260605_082241\remote_build.log
firmware package: <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
burn log: <repo-root>\logs\burn\v4.4.66_direct_20260605_082821\run_summary.txt
three-board test log: <repo-root>\logs\live\v4.4.66_three_board_cfg_relay_20260605_084012\run.log
COM16 status: blocked/no app reply unless new evidence appears
```

## Verification Result

```text
local regression: PASS, 34 unit tests OK
remote Ubuntu build: PASS, package contains v4.4.66
flash COM13: PASS, YMODEM transfer completed
flash COM17: PASS, YMODEM transfer completed
flash COM18: PASS, YMODEM transfer completed
flash COM16: FAIL, no boot handshake before manual retry timeout
serial version confirm COM13: PASS, fw=v4.4.66
serial version confirm COM17: PASS, fw=v4.4.66
serial version confirm COM18: PASS, fw=v4.4.66
three-board relay reboot recovery: PASS
```
