# Version v4.4.3

Date: 2026-05-31

## Positioning

`v4.4.3` is a serial deployment verification patch on top of `v4.4.2`. It keeps the same firmware business behavior, WebUI one-click configuration behavior, and confirmed ST7789 hardware tuple, but fixes the Windows PowerShell helper used to test and configure many nodes over COM ports.

## What Changed

- Fixed `scripts/ws63_serial_cfg.ps1` so the script does not collide the parameter `$Port` with the local serial object variable `$port`. PowerShell variables are case-insensitive, so that collision could make cleanup fail before the board reply was reported.
- Silenced `Add-Type` inside the serial helper so helper setup cannot leak extra pipeline objects into the serial object assignment.
- Added contract coverage for the serial helper implementation shape so the collision is less likely to return.
- Moved firmware-visible version strings and ST7789 title strings from `v4.4.2` to `v4.4.3`.

## Carried Forward

- WebUI one-click config surfaces config `ok=false` replies and preserves valid `channel=0`.
- Serial helper still leaves DTR/RTS off by default; `-UseControlLines` is opt-in only.
- Unified runtime-role firmware, domain WebUI WebSerial support, and serial `cfg` commands remain the deployment path.
- ST7789 remains `240x135`, offset `40,53`, MADCTL `0x60`, software SPI mode 0, with no GPIO11 backlight control.
- SLE advertise TX power and scan-response declaration remain unified at `18 dBm`.

## Known Limits

- GPS remains pinmap/logging only, not full NMEA parsing.
- LVGL remains optional and falls back to the built-in text renderer when the SDK image has no LVGL backend.
