# Version v4.4.48

Date: 2026-06-04

## Scope

`v4.4.48` fixes the live re-test and bulk-deployment edge case where a board that was already running the requested role rejected the same serial/WebUI role configuration with `ret=-4`. That behavior made a correctly running leader/member pair look failed when automation repeated `role leader` or `role member <leader_suffix>`.

## Root Cause

`team_serial_cfg_apply_loaded()` rejected every config apply while `g_team_rt.role_configured` was already set. That protection is correct for real runtime role changes, but it was too strict for idempotent re-application of the same role, team, channel, and leader. During a live COM16/COM13 re-test, COM16 was already leader and exchanging heartbeats, but `role leader` saved NV then returned `ret=-4`, causing the bootstrap test to fail before it could validate member reboot recovery.

## What Changed

1. Added `team_serial_cfg_matches_runtime()` to compare requested NV config against the current runtime role.
2. Allowed `team_serial_cfg_apply_loaded()` to return `SLE_TEAM_OK` when the requested role/team/channel/leader already matches runtime.
3. Preserved the safety boundary: requests that would actually change runtime role, team, channel, or leader still return `SLE_TEAM_ERR_UNSUPPORTED`.
4. Bumped firmware-visible version, build guard, flash guard, README, and contract tests to `v4.4.48`.

## Expected Behavior

- Repeating `role leader` on an already-correct leader returns `ret=0` instead of failing with `ret=-4`.
- Repeating `role member <leader_suffix>` on an already-correct member returns `ret=0`.
- Runtime role changes are still not allowed without clearing/leaving/rebooting into the requested config path.
- Member reboot auto-restore remains the same as `v4.4.47`: only `reboot` or hardware reset is needed after a saved leader exists in flash.
