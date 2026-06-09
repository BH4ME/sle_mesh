# Manifest: v4.4.117

Date: 2026-06-09

## Scope

- WS63 unified firmware version bump to `v4.4.117`.
- ST7789 display CS held low for the FPC workaround.
- ST7789 RS/DC and RESET pin mapping swapped in firmware defaults and build-time Kconfig injection.
- Build, flash, automation, README, and version-index defaults updated to `v4.4.117`.

## Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/Kconfig`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_five_board_member_loss_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `scripts/build/ws63_build_v4_ubuntu.sh`
- `scripts/build/ws63_build_v4_local_wsl.sh`
- `scripts/flash/ws63_flash_multi.ps1`
- `scripts/flash/ws63_flash_team.sh`
- `README.md`
- `firmware/README.md`
- `docs/v4/README.md`
- `docs/version_management.md`
- `versions/README.md`
- `versions/v4.4.117/VERSION.md`
- `versions/v4.4.117/MANIFEST.md`
- `xc/ws63_team_network/README.md`

## Notes

- Intended hardware-facing behavior change: ST7789 CS is held low, RS/DC and RESET are swapped to match the corrected FPC wiring.
- Historical `v4.4.116` records remain unchanged.
