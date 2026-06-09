# Version v4.4.103

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.103`.

## Summary

- Advanced firmware and current repository records from `v4.4.102` to `v4.4.103`.
- Reused the board HTTP query value-start helper for `team_http_query_hex16`.
- Kept the existing `/api/...` endpoints and hex leader-suffix parsing behavior unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.102`.

## Maintenance Notes

- `team_http_query_hex16` now shares the query-key lookup helper used by decimal query parsers.
- It still consumes exactly four hex digits, matching the previous route-suffix parser behavior.
- Historical `v4.4.102` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.103 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.103` marker.
