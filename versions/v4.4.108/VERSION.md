# Version v4.4.108

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.108`.

## Summary

- Advanced firmware and current repository records from `v4.4.107` to `v4.4.108`.
- Replaced the repeated CLI sub-handler `if` chain with a fixed-order handler table loop.
- Kept CLI handler order and early-return behavior unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.107`.

## Maintenance Notes

- The CLI dispatcher still tries WS2812, buzzer, battery, display, LED, and serial config handlers in the same order.
- Role commands, leave handling, and fallback behavior were intentionally left unchanged.
- Historical `v4.4.107` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.108 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.108` marker.
