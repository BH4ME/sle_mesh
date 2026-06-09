# Version v4.4.118

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.118`.

## Summary

- Advanced firmware and current repository records from `v4.4.117` to `v4.4.118`.
- Simplified ST7789 display initialization logging by sharing the repeated pin-map summary print through one helper.
- Preserved the success and failure branches, the emitted ready value, the emitted phase text, and the ST7789 CS-always-low / RS-reset pin-map correction from `v4.4.117`.

## Maintenance Notes

- `team_display_init_log()` emits the same initialization summary fields for both success and failure.
- Successful display init still sets `display_ready=1` and reports `phase=ready`.
- Failed display init still sets `display_ready=0`, reports `phase=failed`, and prints the existing disabled-after-failure line.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, or display pin mapping was intentionally changed.
- Historical `v4.4.117` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.118 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.118` marker and the ST7789 pin-map guard.
