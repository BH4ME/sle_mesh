# Version v4.4.35

Date: 2026-06-02

## Scope

`v4.4.35` is the final logic-check patch after `v4.4.34`. It fixes a real
interface consistency issue found during review: the old role shortcut path was
still separate from the newer one-click `cfg` configuration path.

## Root Cause Evidence

1. The hosted WebUI role shortcut called `configureRole()`, and WiFi mode still
   mapped leader selection to `GET /api/role?role=leader`.
2. WebSerial role shortcut still sent `role leader` / `role member <suffix>`,
   so member shortcut ignored the form's `team` and `channel` values.
3. The board compatibility handler for `GET /api/role?role=leader` queued
   `team_request_role_config()` using `g_team_node.cfg.team_id` and
   `g_team_node.cfg.channel_hash`, which can still be zero while the board is
   unconfigured.
4. The new bulk deployment path was correct, but the shortcut path could still
   bypass it.

## What Changed

1. Firmware-visible version strings now report `v4.4.35`.
2. Hosted WebUI `configureRole()` now calls the unified config API:
   `/api/config/leader?...&now=1` or `/api/config/member?...&now=1`.
3. WebSerial `configureRole()` now sends `cfg leader now 1 17` or
   `cfg member now <leader_suffix> <team> <channel>`.
4. Board compatibility `/api/role?role=leader` now falls back through
   `team_cfg_default_team()` and `team_cfg_default_channel()` instead of reading
   the unconfigured node struct.
5. Contract tests now lock this behavior so the UI cannot silently regress to
   the old shortcut path.

## Verification

- PASS: `npm test` in `webui` (`51/51` tests passed).
- PASS: `npm run build` in `webui`.
- PASS: Remote Ubuntu clean firmware build on
  `owen@192.168.6.5:/home/owen/workspace/bearpi-pico_h3863` using Python
  Paramiko fallback because local `sshpass` was unavailable.
- PASS: Remote post-build guard confirmed `.config`, `.map`, and `.elf` select
  the team-network app, ST7789 display, `sle_team_node`, `v4.4.35`,
  `[display] st7789 ready`, `[team] boot unconfigured`, and `[cfg-json]`.
- PASS: Downloaded firmware package:
  `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
  (`1590376` bytes).
- BLOCKED locally: `scripts/simulate_v2.sh --suite=all --stress=1` could not
  run because this Windows/WSL environment currently has no `cc`, `clang`, or
  `gcc` in PATH.
