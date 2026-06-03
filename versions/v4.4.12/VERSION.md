# Version v4.4.12

Date: 2026-06-01

## Scope

`v4.4.12` focuses on code-structure cleanup for the WS63 runtime entry flow, so
the main task loop is easier to read and maintain.

## Why

`team_network_task` and `team_network_entry` had become long and visually dense.
The logic itself was valid, but initialization and runtime paths were mixed in
one large block, increasing maintenance cost and review difficulty.

## What Changed

1. Firmware version bump:
   - `SLE_TEAM_FW_VERSION`: `v4.4.11 -> v4.4.12`
   - `SLE_TEAM_HW_CONSTRAINTS`: `v4.4.11 board map -> v4.4.12 board map`

2. `team_network_task` refactor (behavior-preserving split):
   - `team_network_wait_identity_ready()`
   - `team_network_task_bootstrap()`
   - `team_network_tick_common()`
   - `team_network_tick_role_configured()`

3. `team_network_entry` refactor:
   - `team_network_prestart()`
   - `team_network_spawn_task()`

4. Safety:
   - added forward declaration for `team_network_task` before task spawn helper.

## Expected Impact

- No protocol/role behavior change intended.
- Same runtime call order, but cleaner flow:
  - prestart/bootstrap
  - common tick
  - role-configured tick
- Easier future maintenance and audits.
