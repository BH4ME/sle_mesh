# Manifest v4.4.93

Date: 2026-06-06

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `include/sle_team_web_api.h`
- `src/sle_team_web_api.c`
- `webui/src/protocol/types.ts`
- `webui/src/api/client.ts`
- `webui/src/main.ts`
- `webui/README.md`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_flash_multi.ps1`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `versions/README.md`
- `versions/v4.4.93/VERSION.md`
- `versions/v4.4.93/MANIFEST.md`
- `meta/PROJECT_OPERATION_SOP.md`

## Notes

This is a code and version-management change for dynamic automatic relay sizing. It does not raise `SLE_TEAM_MAX_LOGICAL_MEMBERS`; that remains the separate RAM-expensive scaling knob.

## Verification

- `.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py`
- `.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -p test_*.py -t .`
- `npm --prefix webui test`
- `npm --prefix webui run build`
- `git diff --check`

All listed non-firmware checks passed locally; `git diff --check` reported CRLF normalization warnings only. Firmware compile/build and hardware flash/burn were not run by user request; a previously-started local WSL build was stopped after the no-compile clarification.
