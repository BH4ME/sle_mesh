# Version v4.4.117

## Type

Firmware hardware pin-map correction release.

## Firmware Version

The WS63 firmware version is now `v4.4.117`.

## Summary

- Advanced firmware and current repository records from `v4.4.116` to `v4.4.117`.
- Updated the ST7789 FPC workaround so the display CS net is held low for the whole runtime.
- Swapped the ST7789 RS/DC and RESET software pin mapping: RS/DC now uses GPIO13 and RESET now uses GPIO10.
- Added a build-time guard for `CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW=y`, RS/DC GPIO13, and RESET GPIO10.

## Hardware Notes

- This release assumes the ST7789 CS input/net must never be driven high.
- The driver still calls the CS helper around SPI writes, but the helper now always drives GPIO low when `CONFIG_SLE_TEAM_ST7789_CS_ALWAYS_LOW` is enabled.
- If the FPC mistake actually ties the panel ground pad to the MCU CS output, this firmware keeps that output low; a real hardware ground is still the safer electrical fix.
- No SLE transport behavior, relay policy, LED/RGB timing, or WebUI command syntax was intentionally changed.
- Historical `v4.4.116` release notes remain unchanged.

## Validation

- Source-level tests should assert the v4.4.117 firmware marker, CS-always-low guard, and swapped display pins.
- Python syntax checks should cover updated automation tools.
- Remote Ubuntu build should confirm the firmware package contains the `v4.4.117` marker and the ST7789 pin-map guard.
