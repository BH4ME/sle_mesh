# v4.4.37 Manifest

## Changed Files

- `.gitmodules`
- `README.md`
- `versions/README.md`
- `versions/v4.4.37/VERSION.md`
- `versions/v4.4.37/MANIFEST.md`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/tests/ws63-api-contract.test.mjs`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch`

## Key Logic Deltas

1. Remote LVGL patching is idempotent when the synced source already contains the C89 fix, and clean LVGL checkouts use `--unidiff-zero`.
2. Version strings and version docs are synchronized to `v4.4.37`.
3. Windows CRLF worktrees no longer make WebUI source contract tests fail.
4. Auto-burn unit tests cover the updated burner constructor shape.

## Verification

Completed before commit/push:

- WebUI contract tests: 52/52 pass.
- WebUI production build: pass.
- Python automation/tools unit tests: 15/15 and 22/22 pass.
- Python 30-node relay failure/recovery simulations: 3/3 and 5/5 pass, `drop=0`, `lost_parent=0`.
- Remote Ubuntu firmware build: pass through paramiko fallback.
- Post-build guards: pass for `v4.4.37`, team-network, ST7789, and serial config markers.
- Diff whitespace/integrity checks: pass with only Windows line-ending warnings.
