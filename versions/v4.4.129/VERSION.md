# Version v4.4.129

## Type

Firmware power-status reporting release.

## Firmware Version

The WS63 firmware version is now `v4.4.129`.

## Summary

- Advanced firmware and current repository records from `v4.4.128` to `v4.4.129`.
- Added TP4054 `CHRG` status input support on WS63 IO2.
- Treats TP4054 `CHRG` as active-low with an external pull-up: low means `pwr-charging`.
- Keeps CHRG high honest as `battery-or-full`, because CHRG alone cannot distinguish battery-only from external PWR with charging complete.
- Added power status fields to UART `bat`/`power`/`pwr` commands.
- Added `/api/power` and a `power` object inside `/api/status`.
- Added VBAT, battery percent, charging, and CHRG raw-state rows to the board status page.

## Hardware Notes

- ADC battery measurement remains ADC_CTRL GPIO5, ADC_VBAT GPIO12, ADC channel 5, divider 390k/100k.
- TP4054 `CHRG` is read-only on GPIO2; firmware does not drive it.
- Because the board already pulls CHRG high externally, firmware leaves the internal pull configuration unchanged.
- A separate VBUS/PWR detect GPIO would be needed to report `pwr` with certainty after the battery is full.

## Validation

- Source-level tests assert the v4.4.129 firmware marker, TP4054 CHRG defaults, power JSON strings, `/api/power`, and archive-preservation logic.
- Build guards require CHRG Kconfig values and power-status strings in the linked firmware.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.129` marker and new power-status guard strings.
