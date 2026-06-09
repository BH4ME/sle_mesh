# Version v4.4.116

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.116`.

## Summary

- Advanced firmware and current repository records from `v4.4.115` to `v4.4.116`.
- Simplified the buzzer GPIO level selection by replacing the active-high/active-low branch with one equivalent expression.
- Preserved the existing muted-buzzer handling, active-high truth table, active-low truth table, and final GPIO level normalization from `v4.4.115`.

## Maintenance Notes

- Active-high still passes the requested level through unchanged.
- Active-low still maps nonzero requested level to GPIO low and zero requested level to GPIO high.
- Muted builds still force the configured off level and clear `buzzer_level_on`.
- No command syntax, SLE transport behavior, relay policy, LED/RGB timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.115` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.116 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.116` marker.
