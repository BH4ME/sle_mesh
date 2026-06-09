# Version v4.4.111

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.111`.

## Summary

- Advanced firmware and current repository records from `v4.4.110` to `v4.4.111`.
- Simplified `team_led_set` by computing the active-low GPIO output level once and calling `uapi_gpio_set_val` once.
- Preserved the existing active-high and active-low truth table: on maps to HIGH/LOW respectively, and off maps to LOW/HIGH respectively.
- Preserved LED CLI commands, blink behavior, pin validation, and status output from `v4.4.110`.

## Maintenance Notes

- The LED output expression is equivalent to the previous active-low branch pair.
- No command syntax, LED event timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.110` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.111 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.111` marker.
