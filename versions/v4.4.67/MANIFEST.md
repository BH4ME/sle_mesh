# v4.4.67 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.67/VERSION.md`
- `versions/v4.4.67/MANIFEST.md`
- `versions/v4.4.67/FLASH_AND_FOUR_BOARD_TEST.md`
- `versions/v4.4.67/COM16_BLOCKER.md`
- `scripts/ws63_flash_multi.ps1`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `meta/PROJECT_OPERATION_SOP.md`

## Expected Firmware Package

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.66`; `v4.4.67` is a workflow/version-record update, not a firmware rebuild.

## Verification Commands

```powershell
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_auto_burn.py automation\ws63\tools\ws63_relay_cycle_test.py automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py
.\.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -t .
git diff --check
```

## Current Verification Evidence

```text
local regression: E:\codex_documents\sle\logs\local\v4.4.67_20260605_092345\local_regression.log
parallel flash: E:\codex_documents\sle\logs\burn\v4.4.66_20260605_085733
status confirm: E:\codex_documents\sle\logs\serial\v4.4.67_parallel_confirm_20260605_090106\confirm.log
COM16 blocker: no cfg-json and no boot handshake in all probes recorded in VERSION.md
```

Additional source-level regression now asserts the relay recovery policy is target-based and that the four-board test reports whether the original relay or child relay keeps the role.

Four-board runtime command:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.66 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --reboot-command "cfg reboot"
```
