# Version v4.4.107

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.107`.

## Summary

- Advanced firmware and current repository records from `v4.4.106` to `v4.4.107`.
- Folded paired LED CLI branches for `led on` and `led off`.
- Kept existing LED set behavior and status output unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.106`.

## Maintenance Notes

- The LED CLI still calls `team_led_cli_status` after both `led on` and `led off`.
- `led tx`, `led rx`, `led active_low`, `led active_high`, `led pin <0-31>`, and help output were intentionally left unchanged.
- Historical `v4.4.106` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.107 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.107` marker.
