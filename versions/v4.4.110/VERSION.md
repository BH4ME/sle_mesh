# Version v4.4.110

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.110`.

## Summary

- Advanced firmware and current repository records from `v4.4.109` to `v4.4.110`.
- Added `team_serial_cfg_cli_save_leader` to share the repeated serial configuration CLI leader-save return path.
- Replaced repeated `cfg leader` save/log/JSON-status bodies with helper calls.
- Preserved all existing command matching, default team/channel selection, log messages, save behavior, and JSON status output order from `v4.4.109`.

## Maintenance Notes

- `cfg leader`, `cfg leader <team> <channel>`, and `cfg leader now <team> <channel>` still use the same accepted ranges and apply-now behavior.
- Member configuration branches were intentionally left local because extracting them did not reduce the source.
- Historical `v4.4.109` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.110 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.110` marker.
