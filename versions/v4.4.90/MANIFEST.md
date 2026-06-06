# Manifest v4.4.90

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `src/sle_team_cli.c`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_multi.ps1`
- `webui/src/api/client.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.90/FLASH_AND_FOUR_BOARD_TEST.md`

## Notes

This version intentionally stops at code, tests, and remote build synchronization. Do not flash from this record unless the user explicitly asks for a burn after reviewing these changes.

## Verification

- Python unit tests: pass, 26 tests.
- WebUI contract tests: pass, 54 tests.
- `python -m py_compile`: pass.
- `git diff --check`: pass, CRLF normalization warnings only.
- Remote Ubuntu firmware build: pass.
- Built package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1603176`
- Package guard: `contains_v4.4.90=True`
- Hardware flash: not run.
