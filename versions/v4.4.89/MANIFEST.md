# Manifest v4.4.89

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_flash_multi.ps1`
- `scripts/ws63_flash_team.sh`
- `README.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/README.md`
- `versions/v4.4.89/VERSION.md`
- `versions/v4.4.89/MANIFEST.md`

## Verification

- `git diff --check`: pass, CRLF warnings only.
- Python `py_compile`: pass.
- Python unit tests: pass, 20 tests.
- Remote Ubuntu build: pass.
- Built package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1602792`
- Package guard: `contains_v4.4.89=True`

Hardware flash/test was intentionally not run for this version. Run it only after an
explicit burn request.
