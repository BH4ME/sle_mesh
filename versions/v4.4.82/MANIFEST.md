# Manifest v4.4.82

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `README.md`
- `versions/README.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/v4.4.82/VERSION.md`
- `versions/v4.4.82/MANIFEST.md`
- `versions/v4.4.82/AUTO_FLASH_AND_FOUR_BOARD_TEST.md`

## Verification Plan

Local checks only for this turn:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_four_board_relay_test.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```

Hardware verification remains required after an explicit burn request:

```text
COM16 leader, COM13/COM17/COM18 members
leader direct cap = 1
initial topology: active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
member reboot/rejoin: PASS
relay reboot failover: PASS
original relay or child relay recovery policy recorded
no route hint/config NO_ROUTE regression
```

## Verification Result

Local checks:

```text
git diff --check: PASS
py_compile: PASS
unit tests: PASS, 19 tests
```

Note: the unit-test output includes an intentional stale-package guard message
from `test_main_refuses_stale_firmware_before_flash`; it is not a real flash
attempt and no COM port was opened.

## Flash Policy

Do not flash automatically for this code correction. Build/burn only after the
user explicitly asks to burn `v4.4.82`.
