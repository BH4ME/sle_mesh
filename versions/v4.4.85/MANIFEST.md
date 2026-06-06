# Manifest v4.4.85

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `scripts/ws63_flash_multi.ps1`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/README.md`
- `versions/v4.4.85/VERSION.md`
- `versions/v4.4.85/MANIFEST.md`

## Verification

Completed verification:

- `git diff --check`: PASS, only Git line-ending warnings.
- Python `py_compile`: PASS.
- Unit tests: PASS, 20 tests.
- Remote Ubuntu build: PASS.
- Package path: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1602024`.
- Package version guard: contains `v4.4.85`.

Repeatable verification:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'E:\codex_documents\sle'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_remote_build_v4.py --host 192.168.6.5 --user owen --password <set-locally> --sdk /home/owen/workspace/bearpi-pico_h3863 --jobs 4
```

## Hardware

Not flashed in this correction. The next hardware validation should build `v4.4.85`
and only then flash after explicit user approval.
