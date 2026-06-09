# Manifest: v4.4.99

## Updated Files

- `xc/ws63_team_network/Kconfig`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/build/ws63_build_v4_local_wsl.sh`
- `scripts/build/ws63_build_v4_ubuntu.sh`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `scripts/flash/ws63_flash_multi.ps1`
- `scripts/flash/ws63_flash_team.sh`
- `README.md`
- `docs/version_management.md`
- `docs/COM14_RTS_DTR_RESET_ISSUE.md`
- `docs/v4/README.md`
- `firmware/README.md`
- `versions/README.md`
- `versions/v4.4.99/VERSION.md`
- `versions/v4.4.99/MANIFEST.md`

## Hardware Basis

- `SCH_Schematic1_2_2026-06-07.pdf`

## Validation Commands

```powershell
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_four_board_relay_test
python -m py_compile automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tools\ws63_four_board_relay_test.py
```

Also validated `scripts/flash/ws63_flash_multi.ps1` with the PowerShell parser and checked Bash syntax for the build/flash helper scripts.

Remote firmware build should also pass the v4.4.99 post-build guard before flashing.
