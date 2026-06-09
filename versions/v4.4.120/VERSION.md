# Version v4.4.120

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.120`.

## Summary

- Advanced firmware and current repository records from `v4.4.119` to `v4.4.120`.
- Added `team_cli_match2()` to share exact two-command CLI matching used by battery/ADC aliases, RGB status, buzzer commands, display status, LED commands, and config status.
- Preserved every accepted command string, helper side effect, command ordering relative to `sscanf` fallbacks, and all ST7789 / ADC / buzzer hardware mappings from `v4.4.119`.

## Maintenance Notes

- `team_cli_match2()` only wraps two `strcmp()` equality checks.
- Numeric command parsing remains guarded by the same `sscanf()` expressions and ranges.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, or display pin mapping was intentionally changed.
- Historical `v4.4.119` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.120 firmware marker and CLI match helper guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.120` marker and existing WS63 source guards.
