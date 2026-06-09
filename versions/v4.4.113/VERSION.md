# Version v4.4.113

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.113`.

## Summary

- Advanced firmware and current repository records from `v4.4.112` to `v4.4.113`.
- Simplified the muted buzzer CLI branch by sharing the forced-off action for `buzz off` and muted sound commands.
- Preserved the existing command matching, buzzer-off behavior, and distinct CLI log text for `buzz off` versus ignored muted sound commands from `v4.4.112`.

## Maintenance Notes

- The merged branch still calls `team_buzzer_set(0U)` for every command path that previously did so.
- `buzz off` still prints the forced-off muted log, while `buzz on`, `buzz beep`, `buzz test`, and valid `buzz beep <ms>` still print the muted ignored log.
- No command syntax, SLE transport behavior, relay policy, LED/RGB timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.112` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.113 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.113` marker.
