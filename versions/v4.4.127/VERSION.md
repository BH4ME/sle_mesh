# Version v4.4.127

## Type

Firmware behavior tuning release.

## Firmware Version

The WS63 firmware version is now `v4.4.127`.

## Summary

- Advanced firmware and current repository records from `v4.4.126` to `v4.4.127`.
- Reduced the built-in WS2812/RGB default brightness again for boot, idle, leader, member, seek, tx, rx, warn, error, fixed-color CLI commands, and the RGB test pattern.
- Made the normal base states breathe: idle, leader, member, and error now render through the breathing animation instead of staying solid.
- Kept the same RGB state names, event flash timing, CLI command names, manual `rgb set <r> <g> <b>`, and ST7789 / ADC / buzzer hardware mappings from `v4.4.126`.
- Preserved firmware build archives by keeping versioned package copies while still updating the existing latest package path for flash scripts.

## Maintenance Notes

- The main WS2812 color cap moved from `16` to `8`; proportional values moved from `12` to `6`, from `10` to `5`, and from `6` to `3`.
- The breathing minimum scale is `0`, so breathing states can fade to off before rising again.
- Manual `rgb set <r> <g> <b>` remains available for explicit operator-selected values.
- Historical `v4.4.126` release notes remain unchanged.

## Validation

- Source-level tests assert the v4.4.127 firmware marker, dimmed WS2812 constants, breathing base states, and firmware archive-preservation logic.
- Python syntax checks cover updated automation tools.
- Remote Ubuntu build confirmed the firmware package contains the `v4.4.127` marker and existing WS63 source guards.
