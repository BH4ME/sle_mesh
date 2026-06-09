# Version v4.4.105

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.105`.

## Summary

- Advanced firmware and current repository records from `v4.4.104` to `v4.4.105`.
- Folded repeated unmuted buzzer CLI toggle handling for `buzz beep`, `buzz test`, and `buzz beep <ms>` into one branch.
- Kept existing buzzer command behavior, muted-firmware handling, and accepted `buzz beep <ms>` range unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.104`.

## Maintenance Notes

- The unmuted buzzer CLI still toggles once and prints the same `buzz toggled` log for all three toggle command forms.
- `buzz on`, `buzz off`, muted command handling, disabled-Kconfig handling, and help output were intentionally left unchanged.
- Historical `v4.4.104` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.105 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.105` marker.
