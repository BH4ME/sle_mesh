# Version v4.4.115

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.115`.

## Summary

- Advanced firmware and current repository records from `v4.4.114` to `v4.4.115`.
- Simplified the fixed-color RGB CLI commands by merging `rgb red`, `rgb green`, `rgb blue`, and `rgb white` into one exact-match branch.
- Preserved the existing accepted command strings, RGB values, helper call, and CLI log text from `v4.4.114`.

## Maintenance Notes

- The merged branch still accepts only the four exact fixed-color commands before deriving the color values.
- The color values remain red `64,0,0`, green `0,64,0`, blue `0,0,64`, and white `40,40,40`.
- No command syntax, SLE transport behavior, relay policy, LED/RGB timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.114` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.115 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.115` marker.
