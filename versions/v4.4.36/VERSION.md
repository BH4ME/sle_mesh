# Version v4.4.36

Date: 2026-06-03

## Scope

`v4.4.36` is a final review-fix patch for the v4.4 unified firmware and WebUI.
It addresses issues found before GitHub upload and verification.

## Root Cause Evidence

1. Persistent config saves logged `uapi_nv_flush()` but returned success when only `uapi_nv_write()` succeeded.
2. The hosted/WebSerial leader shortcut still used hardcoded `team=1` and `channel=17`, while member config used form values.
3. The firmware build now requires LVGL, but LVGL was only present as an untracked nested checkout.
4. Build guards and documentation still pointed at `v4.4.35`.

## What Changed

1. Firmware-visible version strings now report `v4.4.36`.
2. Web config save, config clear, and leader allowlist save return OK only when both NV write and NV flush succeed.
3. WebUI leader quick config now requires and sends the same Team/Channel fields as member config.
4. LVGL is documented as a Git submodule and the WS63 C89 compatibility patch is stored under `xc/ws63_team_network/third_party/lvgl-patches/`.
5. The remote Ubuntu build script applies the LVGL patch idempotently and verifies the `v4.4.36` ELF marker.

## Verification Plan

- `npm --prefix webui test`
- `npm --prefix webui run build`
- `python tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 14 --stress 3`
- `python tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 14 --stress 5 --batch-fail-relay-count 2 --batch-fail-relay-ticks 6,8`
- `git diff --check`

## Notes

The local LVGL checkout may remain dirty while the patch is being tested. The repository-level source of truth is the submodule pointer plus `lv8.3.11-ws63-c89-rect.patch`.
