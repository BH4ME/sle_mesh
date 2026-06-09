# Manifest: v4.4.101

Date: 2026-06-09

## Scope

- WS63 unified firmware version bump to `v4.4.101`.
- WS2812 application-level status animation changes.
- Build, flash, automation, README, and version-index defaults updated to `v4.4.101`.

## Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
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
- `versions/v4.4.101/VERSION.md`
- `versions/v4.4.101/MANIFEST.md`
- `xc/ws63_team_network/README.md`

## Notes

- Historical `v4.4.100` records remain unchanged.
- Runtime behavior still uses the existing single-pixel WS2812 driver; this release changes the application state machine above it.
