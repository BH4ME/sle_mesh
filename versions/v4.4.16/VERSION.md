# Version v4.4.16

Date: 2026-06-01

## Scope

`v4.4.16` fixes member rejoin failure after power-cycle in allowlist mode.

## Root Cause

Leader-side allowlist check treated `member_filter_enabled=1` with
`allowed_member_count=0` as deny-all, so rejoined members were rejected with:
`member rejected by allowlist`.

## What Changed

1. Protocol logic fix:
   - `src/sle_team_node.c`
   - `sle_team_node_is_member_allowed()` now treats empty allowlist as
     non-blocking (allow) to avoid lockout/deadlock.

2. Firmware version bump:
   - `SLE_TEAM_FW_VERSION`: `v4.4.15 -> v4.4.16`
   - `SLE_TEAM_HW_CONSTRAINTS`: `v4.4.15 board map -> v4.4.16 board map`

3. Version-management sync:
   - root/module/version index updated to `v4.4.16`

## Expected Impact

- Member unplug/replug can rejoin leader automatically instead of staying in
  `on 0 / off 1` due to false allowlist rejection.
- Existing explicit allowlist (count>0) behavior is unchanged.
