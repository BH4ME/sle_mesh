# Version v4.4.47

Date: 2026-06-04

## Scope

`v4.4.47` fixes the live-board blocker where first-time leader/member serial configuration was rejected even though NV write succeeded. This restores the intended behavior: after a member has a saved leader in flash, a member reboot should restore that leader from NV and auto-rejoin without another `role member` or `join` command.

## Root Cause

On COM16, `role leader` logged `uapi_nv_write()` success (`ret=0x0`) followed by `uapi_nv_flush()` warning `0x80000002`. The firmware returned failure when either write or flush was non-success, so the runtime role was not applied and the board stayed unconfigured. With no valid runtime/NV role active, a later member reboot could not exercise the automatic flash-restore path.

This is the same class of issue documented in `v4.4.5`: the write result is authoritative for these WS63 NV calls, while flush is retained as a diagnostic warning.

## What Changed

1. `team_nv_config_save()`, `team_nv_config_clear()`, and `team_nv_allowed_save_from_node()` now return success when `uapi_nv_write()` succeeds, while still logging `flush_ret`.
2. WebUI contract tests now lock the non-fatal flush behavior so the regression cannot be reintroduced.
3. Firmware, build, flash, and direct-burn version guards now target `v4.4.47`.

## Expected Behavior

- First-time `role leader` / `role member <leader_suffix>` can return `ret=0` even if `flush=0x80000002` is logged.
- After one successful member configuration, member reboot only needs the `reboot` command or hardware reset; firmware should load `[team-nv] restore member ...` and retry HELLO automatically.
- Manual `leave` still clears saved config and intentionally prevents auto-rejoin until the member is configured again.
