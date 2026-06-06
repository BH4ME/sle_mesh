# Manifest v4.4.77

Date: 2026-06-05

## Changed Files

- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `examples/team_network_demo.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `scripts/ws63_flash_multi.ps1`
- `README.md`
- `versions/README.md`
- `meta/PROJECT_OPERATION_SOP.md`

## Verification Plan

Local/source checks:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py
.\.tooling\py311\python.exe -m unittest automation.ws63.tests.test_ws63_four_board_relay_test automation.ws63.tests.test_ws63_auto_burn
```

Remote build:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<local only>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

Expected package after build:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

## Flash Policy

No flash is part of this code review step. When flashing is requested, use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.77 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Notes

This version intentionally does not change the target relay policy. With
`direct cap=1`, the leader keeps one direct member and migrates the other online
members behind relay candidates. Original relay recovery remains target based:
if only one relay is required, the current best relay remains relay and any
extra relay is demoted by the normal rebalance policy.
