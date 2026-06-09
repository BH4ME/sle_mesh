# Version v4.4.124

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.124`.

## Summary

- Advanced firmware and current repository records from `v4.4.123` to `v4.4.124`.
- Simplified `team_cli_match2()` by returning the logical match result directly as `uint8_t` instead of using a `? 1U : 0U` tail.
- Preserved the same two-command exact matching behavior, helper call sites, CLI/HTTP command syntax, and ST7789 / ADC / buzzer hardware mappings from `v4.4.123`.

## Maintenance Notes

- The helper still returns `1` when either accepted command string matches and `0` otherwise; C logical operators produce a 0/1 result before the explicit `uint8_t` cast.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, saved config behavior, or display pin mapping was intentionally changed.
- Historical `v4.4.123` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.124 firmware marker and direct `team_cli_match2()` boolean return guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.124` marker and existing WS63 source guards.
