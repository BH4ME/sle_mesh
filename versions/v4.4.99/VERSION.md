# Version v4.4.99

## Type

Firmware and repository release.

## Firmware Version

The WS63 firmware version is now `v4.4.99`.

## Summary

- Updated the v3.2 schematic pinmap for the WS2812 status LED, buzzer, GPS UART, battery ADC pins, and ST7789 display.
- Enabled boot-time hardware initialization reporting over serial with present/ready/pin summaries.
- Added an explicit display `phase=ready` hardware line when ST7789 initialization finishes after the boot summary.
- Kept the buzzer initialized but muted by default, including CLI sound commands.
- Added WS2812 RGB state colors for boot, idle, leader, member, seeking, TX, RX, warning, and error states.
- Made the default COM14 burn path use 1024-byte YMODEM with `nonblocking-drain`, not serial write chunking.
- Updated build scripts and post-build guards so generated firmware uses the v3.2 pinmap.
- Advanced repository and firmware version records from `v4.4.98` / `v4.4.95` to `v4.4.99`.

## Pinmap

- WS2812 RGB: `RGB` -> WS63 `IO0`.
- Buzzer: `BUZZ` -> WS63 `IO14`, active high, muted by default.
- GPS: UART1, `U1TX` -> WS63 `IO17`, `U1RX` -> WS63 `IO18`.
- ADC: `ADC_CTRL` -> WS63 `IO05`, `ADC_VBAT` -> WS63 `IO12`.
- ST7789 display: `SCL` -> `IO07`, `SDA` -> `IO09`, `CS` -> `IO08`, `RS` -> `IO10`, `RESET` -> `IO13`.

## Validation

- Source-level tests updated to assert the v3.2 pinmap, muted buzzer policy, boot hardware report, and `v4.4.99` firmware marker.
- Burn-tool tests updated to assert the default non-chunked `nonblocking-drain` serial write mode.
- Build scripts now guard the Kconfig pin values and ELF strings for the hardware report and RGB state logging.
