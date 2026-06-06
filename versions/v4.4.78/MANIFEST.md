# Manifest v4.4.78

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
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

## Verification Completed

Completed on 2026-06-05 after the `v4.4.74` feedback review:

```text
git diff --check: PASS
py_compile ws63 automation tools: PASS
unit tests: PASS, 15 tests OK
package version guard: PASS, package contains v4.4.78
package size: 1600936 bytes
package path: output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The local embedded Python runtime does not automatically add the repository root
to `sys.path`, so module-style unit tests must be run with the repo root inserted
explicitly or through an equivalent wrapper. This is an environment issue, not a
firmware logic failure.

## Flash Policy

Do not flash as part of this code-fix step. When flashing is explicitly
requested, use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.78 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Notes

This version changes the physical convergence path, not only the logical route
state. The important runtime proof after flashing will be:

```text
child:  parent reselect drop old leader conn=...
leader: direct cap prune disconnect member=... no-offline
leader: route metrics active=3 direct=1 relayed=2 converged=1
```
