# Manifest v4.4.79

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

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```

Remote build:

```powershell
$env:UBUNTU_HOST='192.168.6.5'
$env:UBUNTU_USER='owen'
$env:UBUNTU_PASS='<local only>'
$env:UBUNTU_SDK='/home/owen/workspace/bearpi-pico_h3863'
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py
```

## Runtime Proof Required

After flashing:

```text
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
relay reboot: child relay elected
original relay return: original relay rejoins through the selected relay, no repeated HELLO 241->154 NO_ROUTE
final members: 241, 224, 86 online
```

## Flash Policy

Use the documented parallel wrapper:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.79 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```
