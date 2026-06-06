# v4.4.68 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.68/VERSION.md`
- `versions/v4.4.68/MANIFEST.md`
- `versions/v4.4.68/FLASH_AND_FOUR_BOARD_TEST.md`
- `versions/v4.4.68/COM16_BLOCKER.md`
- `scripts/ws63_flash_multi.ps1`
- `meta/PROJECT_OPERATION_SOP.md`

## Expected Firmware Package

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.66`; `v4.4.68` is not a firmware rebuild.

## Verification Commands

```powershell
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_auto_burn.py automation\ws63\tools\ws63_relay_cycle_test.py automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py
.\.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -t .
git diff --check
```

## Evidence

```text
local regression: E:\codex_documents\sle\logs\local\v4.4.68_20260605_094309\local_regression.log
staggered parallel flash: E:\codex_documents\sle\logs\burn\v4.4.66_20260605_094335\run_summary.txt
status confirm: E:\codex_documents\sle\logs\serial\v4.4.68_staggered_parallel_confirm_20260605_094640\confirm.log
COM16 blocker: no cfg-json and no boot handshake in v4.4.67 probes
```
