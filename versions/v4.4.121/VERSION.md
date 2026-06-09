# Version v4.4.121

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.121`.

## Summary

- Advanced firmware and current repository records from `v4.4.120` to `v4.4.121`.
- Added `team_cfg_apply_role()` to share the repeated runtime role-apply config construction used after saved leader/member config succeeds.
- Preserved the existing NV-save entry points, apply-now gating, role values, team/channel values, leader suffix values, CLI/HTTP command syntax, and ST7789 / ADC / buzzer hardware mappings from `v4.4.120`.

## Maintenance Notes

- `team_cfg_save_leader()` still saves leader NV first and only applies after `ret == SLE_TEAM_OK` and `apply_now != 0U`.
- `team_cfg_save_member()` keeps the same member suffix validation path at its callers and the same apply-now behavior.
- No SLE transport behavior, relay policy, LED/RGB timing, WebUI command syntax, ADC scaling, buzzer logic, or display pin mapping was intentionally changed.
- Historical `v4.4.120` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.121 firmware marker and config-apply helper guard.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.121` marker and existing WS63 source guards.
