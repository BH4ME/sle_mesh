# Manifest: v4.4.128

Date: 2026-06-09

## Scope

- WS63 unified firmware version bump to `v4.4.128`.
- WS2812 data timing changed from TCXO API polling to RISC-V cycle-counter waits inside the IRQ-locked transmit burst.
- WS2812 boot diagnostic now emits short red, green, and blue pulses at level `32`, then returns to dim normal states.
- ST7789 display init now drives CS low before the rest of display pin setup and keeps RESET high before the reset pulse.
- Build, flash, automation, README, and version-index defaults updated to `v4.4.128`.
- Firmware build outputs preserve versioned archive copies and continue updating the latest package path.

## Files

- `xc/ws63_team_network/src/ws63_ws2812.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
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
- `versions/v4.4.128/VERSION.md`
- `versions/v4.4.128/MANIFEST.md`
- `xc/ws63_team_network/README.md`

## Notes

- Intended behavior changes are limited to WS2812 signal generation, a visible boot RGB diagnostic, and safer display pin priming for the current FPC wiring.
- Historical `v4.4.127` records remain unchanged.
