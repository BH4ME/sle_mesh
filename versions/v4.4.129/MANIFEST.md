# Manifest: v4.4.129

Date: 2026-06-09

## Scope

- WS63 unified firmware version bump to `v4.4.129`.
- TP4054 CHRG status input added on GPIO2, active-low, external pull-up.
- Battery ADC voltage and percent now appear together with CHRG/source status in UART and Web.
- `/api/power` added and `/api/status` now embeds a `power` object.
- Status page now shows Power, VBAT, Battery, Charging, and CHRG IO2 rows.
- Build, flash, automation, README, and version-index defaults updated to `v4.4.129`.
- Firmware build outputs preserve versioned archive copies and continue updating the latest package path.

## Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
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
- `versions/v4.4.129/VERSION.md`
- `versions/v4.4.129/MANIFEST.md`
- `xc/ws63_team_network/README.md`

## Notes

- Existing ST7789 pin mapping and WS2812 behavior from `v4.4.128` remain unchanged.
- CHRG low is reported as `pwr-charging`; CHRG high is reported as `battery-or-full` with `sourceCertain=false`.
