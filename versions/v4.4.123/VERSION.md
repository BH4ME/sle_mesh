# Version v4.4.123

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.123`.

## Summary

- Advanced firmware and current repository records from `v4.4.122` to `v4.4.123`.
- Simplified the HTTP query `u8`, `u16`, and `i32` wrappers by checking `team_http_query_number()` directly instead of storing a temporary `ret`.
- Preserved the same parser helper call arguments, failure return path, output casts, CLI/HTTP command syntax, and ST7789 / ADC / buzzer hardware mappings from `v4.4.122`.

## Maintenance Notes

- The wrapper cleanup is behavior-equivalent: each function still returns `-1` on invalid output storage or failed numeric parsing, then writes the parsed value only after the helper succeeds.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, saved config behavior, or display pin mapping was intentionally changed.
- Historical `v4.4.122` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.123 firmware marker and direct HTTP query wrapper guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.123` marker and existing WS63 source guards.
