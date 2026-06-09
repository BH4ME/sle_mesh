# Version v4.4.122

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.122`.

## Summary

- Advanced firmware and current repository records from `v4.4.121` to `v4.4.122`.
- Simplified the member runtime-match tail in `team_serial_cfg_matches_runtime()` to one equivalent boolean return.
- Preserved the same team/channel pre-checks, leader-role path, member-role validation, leader-id derivation, CLI/HTTP command syntax, and ST7789 / ADC / buzzer hardware mappings from `v4.4.121`.

## Maintenance Notes

- The new return expression is equivalent to the previous branch: member runtime matches only when the runtime role is member and the runtime leader id equals the id derived from the saved leader suffix.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, saved config behavior, or display pin mapping was intentionally changed.
- Historical `v4.4.121` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.122 firmware marker and runtime-match expression guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.122` marker and existing WS63 source guards.
