# Version v4.4.125

## Type

Firmware behavior tuning release.

## Firmware Version

The WS63 firmware version is now `v4.4.125`.

## Summary

- Advanced firmware and current repository records from `v4.4.124` to `v4.4.125`.
- Reduced the built-in WS2812/RGB brightness for boot, idle, leader, member, seek, tx, rx, warn, error, fixed-color CLI commands, and the RGB test pattern.
- Kept the same RGB states, state transitions, flash timing, breathe timing, CLI command names, and ST7789 / ADC / buzzer hardware mappings from `v4.4.124`.
- Preserved firmware build archives by keeping versioned package copies while still updating the existing latest package path for flash scripts.

## Maintenance Notes

- The main WS2812 color cap moved from `64` to `32`; proportional values moved from `48` to `24`, from `40` to `20`, and from `24` to `12`.
- Manual `rgb set <r> <g> <b>` remains available for explicit operator-selected values.
- Historical `v4.4.124` release notes remain unchanged.

## Validation

- Source-level tests assert the v4.4.125 firmware marker, dimmed WS2812 constants, and firmware archive-preservation logic.
- Python syntax checks cover updated automation tools.
- Remote Ubuntu build confirmed the firmware package contains the `v4.4.125` marker and existing WS63 source guards.
