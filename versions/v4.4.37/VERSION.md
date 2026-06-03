# Version v4.4.37

Date: 2026-06-03

## Scope

`v4.4.37` is a build-integrity patch after the v4.4.36 review fixes.
It keeps the unified firmware/WebUI behavior and fixes the remote Ubuntu LVGL patch idempotence gate found during final verification.

## Root Cause Evidence

1. The remote Ubuntu build failed before SDK compilation while applying `lv8.3.11-ws63-c89-rect.patch`.
2. The local LVGL checkout already contained the C89 fix, so the synced remote source could already be in the patched state.
3. The build script only relied on `git apply --check` and `git apply --reverse --check`; in the synced source layout this did not reliably detect the already-patched state.

## What Changed

1. Firmware-visible version strings now report `v4.4.37`.
2. The remote Ubuntu build script now checks the LVGL source marker before applying the zero-context patch with `--unidiff-zero`.
3. WebUI contract tests now normalize CRLF source reads so Windows worktrees do not break source-snippet checks.
4. The firmware C source no longer starts with a UTF-8 BOM.
5. The auto-burn unit-test fake accepts the new burner timeout keyword arguments.

## Verification Results

- `npm --prefix webui test`: pass, 52/52.
- `npm --prefix webui run build`: pass.
- `python -m unittest discover -s automation/ws63/tests -p 'test*.py'`: pass, 15/15.
- `python -m unittest discover -s tools -p 'test*.py'`: pass, 22/22.
- `python tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 14 --stress 3`: pass, 3/3, `drop=0`, `lost_parent=0`.
- `python tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 14 --stress 5 --batch-fail-relay-count 2 --batch-fail-relay-ticks 6,8`: pass, 5/5, `drop=0`, `lost_parent=0`.
- Remote Ubuntu firmware build via paramiko fallback: pass.
- Post-build guard: pass, final ELF contains `v4.4.37`, `[display] st7789 ready`, `[team] boot unconfigured`, and `[cfg-json]`; map contains `ws63_team_network_app.c.obj`, `ws63_st7789_display.c.obj`, and `sle_team_node.c.obj`.
- `git diff --check`: pass with only Windows LF-to-CRLF warnings.

## Notes

The LVGL submodule is pinned to `v8.3.11` and the repository-level patch remains tracked under `xc/ws63_team_network/third_party/lvgl-patches/`.
