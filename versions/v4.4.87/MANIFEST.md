# Manifest v4.4.87

Date: 2026-06-05

## Changed Files

- `src/sle_team_node.c`
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
- `versions/v4.4.87/VERSION.md`
- `versions/v4.4.87/MANIFEST.md`

## Verification

Completed verification:

- `git diff --check`: PASS, only Git line-ending warnings.
- Python `py_compile`: PASS.
- Python unit tests: PASS, 20 tests.
- Remote Ubuntu build: PASS.
- Package path: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1602344`.
- Package version guard: contains `v4.4.87`.

Pending:

- Four-board hardware validation

## Hardware

Not flashed in this correction. The next hardware validation must build and flash
`v4.4.87` only after local checks pass.
