# Manifest v4.4.91

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_multi.ps1`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.91/FLASH_AND_FOUR_BOARD_TEST.md`
- `meta/PROJECT_OPERATION_SOP.md`

## Notes

This is a code-review correction, not a burn record. It exists to close a remaining adapter-layer parsing risk found while following the `v4.4.74` feedback trail.

## Verification

- Python `py_compile`: pass.
- Python unit tests: pass, 27 tests.
- WebUI contract tests: pass, 54 tests.
- `git diff --check`: pass, CRLF normalization warnings only.
- Remote Ubuntu firmware build: pass.
- Built package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1603176`.
- Package guard: `contains_v4.4.91=True`.
- Hardware flash: not run.
