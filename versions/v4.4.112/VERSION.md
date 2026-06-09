# Version v4.4.112

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.112`.

## Summary

- Advanced firmware and current repository records from `v4.4.111` to `v4.4.112`.
- Simplified `team_handle_role_request_once` by merging the identical successful leader/member RGB refresh and buzzer-off cleanup.
- Preserved the existing role request flow, saved configuration behavior, member initial hello, and CLI/Web role configuration entry points from `v4.4.111`.

## Maintenance Notes

- The merged branch still runs only when role configuration succeeds and the requested role is leader or member.
- No command syntax, SLE transport behavior, relay policy, LED/RGB timing, or hardware pin configuration was intentionally changed.
- Historical `v4.4.111` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.112 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.112` marker.
