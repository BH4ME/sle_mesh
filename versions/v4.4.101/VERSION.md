# Version v4.4.101

## Type

Firmware and repository release.

## Firmware Version

The WS63 firmware version is now `v4.4.101`.

## Summary

- Advanced firmware and current repository records from `v4.4.100` to `v4.4.101`.
- Reworked WS2812 status behavior around pairing, role state, member loss, and recovery.
- Added a white breathing LED state for unconfigured/unpaired boards.
- Made configured leaders show green and configured joined members show blue as their steady role colors.
- Made leader member-loss events trigger several fast red flashes.
- Made leader member join/rejoin events trigger several fast green flashes.
- Made disconnected members show a persistent red breathing LED until they reconnect or return to unconfigured mode after factory reset.
- Stopped ordinary TX/RX/seek LED events from overriding the WS2812 role/loss status colors.

## WS2812 Status Policy

- Unconfigured or factory-reset board: white breathing.
- Leader: green steady.
- Joined member: blue steady.
- Member disconnected from leader/upstream: red breathing.
- Leader sees member timeout/loss: red fast flash, then return to green steady.
- Leader sees member join/rejoin: green fast flash, then return to green steady.

## Validation

- Source-level tests assert the v4.4.101 firmware marker and the WS2812 color, breathing, and flash-state guards.
- Build scripts guard the v4.4.101 ELF marker plus the new RGB status `state=%s flash=%s` string and WS2812 animation source markers.
- Flash and automation defaults now expect `v4.4.101`.
