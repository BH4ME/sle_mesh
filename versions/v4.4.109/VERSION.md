# Version v4.4.109

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.109`.

## Summary

- Advanced firmware and current repository records from `v4.4.108` to `v4.4.109`.
- Added `team_serial_cfg_cli_done` to share the repeated serial configuration CLI JSON-status return path.
- Replaced repeated `team_serial_cfg_print_json(); return 1;` pairs in the serial configuration CLI handler.
- Preserved all existing command matching, log messages, configuration writes, and JSON status output order from `v4.4.108`.

## Maintenance Notes

- The `cfg` CLI branches still print their original branch-specific log before emitting the JSON status line.
- No command syntax, accepted range, or apply-now behavior was intentionally changed.
- Historical `v4.4.108` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.109 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.109` marker.
