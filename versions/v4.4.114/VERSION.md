# Version v4.4.114

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.114`.

## Summary

- Advanced firmware and current repository records from `v4.4.113` to `v4.4.114`.
- Simplified the unmuted buzzer CLI `buzz on` and `buzz off` branches into one shared set-and-log path.
- Preserved the existing command matching, `team_buzzer_set` values, and distinct CLI log text for `buzz on` versus `buzz off` from `v4.4.113`.

## Maintenance Notes

- The merged branch still calls `team_buzzer_set(1U)` for `buzz on` and `team_buzzer_set(0U)` for `buzz off`.
- The emitted CLI text remains the same for both commands.
- No command syntax, SLE transport behavior, relay policy, LED/RGB timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.113` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.114 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.114` marker.
