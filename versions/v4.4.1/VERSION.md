# Version v4.4.1

Date: 2026-05-31

## Positioning

`v4.4.1` is a process and traceability patch on top of the confirmed-good `v4.4` firmware line. It does not intentionally change the networking, serial configuration, WebUI, or ST7789 hardware behavior from `v4.4`; it makes the project safer to keep changing by adding a mandatory operation SOP and by moving the visible firmware version forward.

## What Changed

- Added `meta/PROJECT_OPERATION_SOP.md` as the required pre-change operating document.
- Documented the remote Ubuntu build flow, COM16 auto-flash flow, ST7789 fix, serial bulk configuration, required verification, and rollback rules in one place.
- Updated the current project version from `v4.4` to `v4.4.1`.
- Updated firmware-visible version strings so boot logs and the ST7789 UI can identify the flashed build.
- Updated version index and README entry points so future work starts from the new version instead of accidentally continuing on `v4.4`.

## Carried Forward From v4.4

- Unified runtime-role firmware: every WS63 node flashes the same `.fwpkg`.
- Runtime leader/member configuration over firmware HTTP API and serial `cfg` commands.
- External/domain WebUI WebSerial one-click configuration.
- Confirmed ST7789 parameters: `240x135`, offset `40,53`, MADCTL `0x60`, software SPI mode 0.
- BLK/backlight remains hardware/default-on; firmware must not reintroduce GPIO11 backlight control.
- SLE advertise TX power and scan-response declaration remain unified at `18 dBm`.

## Known Limits

- This patch records and enforces process; it is not a new radio/network feature release.
- LVGL remains optional. If the SDK image has no LVGL headers, firmware falls back to the built-in text renderer.
- GPS remains pinmap/logging only, not full NMEA/GPS parsing.
