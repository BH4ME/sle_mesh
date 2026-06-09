# Version v4.4.106

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.106`.

## Summary

- Advanced firmware and current repository records from `v4.4.105` to `v4.4.106`.
- Folded paired LED CLI branches for `led tx`/`led rx` and `led active_low`/`led active_high`.
- Kept existing LED blink constants, status output, and active-level configuration behavior unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.105`.

## Maintenance Notes

- The LED CLI still prints status after `led tx` and `led rx`.
- `led on`, `led off`, `led pin <0-31>`, and help output were intentionally left unchanged.
- Historical `v4.4.105` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.106 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.106` marker.
