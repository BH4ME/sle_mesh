# Version v4.4.102

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.102`.

## Summary

- Advanced firmware and current repository records from `v4.4.101` to `v4.4.102`.
- Reduced duplicated board HTTP query parsing in `ws63_team_network_app.c`.
- Kept the existing `/api/...` query wrapper functions and endpoint behavior unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.101`.

## Maintenance Notes

- `team_http_query_u8`, `team_http_query_u16`, and `team_http_query_i32` now share a bounded integer parser.
- Decimal delimiter handling, range checks, signed latitude/longitude limits, and unsigned `u8`/`u16` parse limits are preserved.
- Historical `v4.4.101` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.102 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
