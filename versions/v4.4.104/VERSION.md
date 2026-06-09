# Version v4.4.104

## Type

Firmware and repository maintenance release.

## Firmware Version

The WS63 firmware version is now `v4.4.104`.

## Summary

- Advanced firmware and current repository records from `v4.4.103` to `v4.4.104`.
- Folded repeated WS2812 CLI fixed-color handling into a shared `team_ws2812_cli_set_rgb` helper.
- Kept the existing `rgb red`, `rgb green`, `rgb blue`, and `rgb white` command behavior and log output shape unchanged.
- Preserved the v3.2 schematic pinmap, muted buzzer policy, WS2812 status behavior, ADC battery sampling, and board WebUI behavior from `v4.4.103`.

## Maintenance Notes

- The fixed-color CLI branches now share the same RGB set-and-log path.
- The `rgb set`, `rgb off`, `rgb test`, `rgb status`, and `rgb help` branches were intentionally left unchanged.
- Historical `v4.4.103` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.104 firmware marker and existing WS63 guards.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.104` marker.
