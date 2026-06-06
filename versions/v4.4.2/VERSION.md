# Version v4.4.2

Date: 2026-05-31

## Positioning

`v4.4.2` is a config-deployment correctness patch on top of `v4.4.1`. It keeps the confirmed `v4.4` ST7789 pinmap/display behavior and unified runtime-role firmware line, while tightening the WebUI and serial helper paths used for bulk node deployment.

## What Changed

- WebUI one-click config now treats firmware or serial `ok=false` config replies as real failures and surfaces the returned `ret` code.
- WebUI one-click config now preserves valid numeric zero values, especially `channel=0`, instead of replacing them with fallback defaults.
- `scripts/ws63_serial_cfg.ps1` no longer toggles DTR or RTS by default. Use `-UseControlLines` only when a board/adapter explicitly needs those control lines.
- Firmware-visible version strings and ST7789 title strings moved from `v4.4.1` to `v4.4.2`.
- Version index, root README, board README, and contract tests now point at `v4.4.2`.

## Carried Forward

- Unified firmware package for all WS63 nodes; leader/member role is selected at runtime by WebUI or serial `cfg` commands.
- Domain/external WebUI supports WiFi HTTP API and WebSerial one-click node configuration.
- ST7789 stays at `240x135`, offset `40,53`, MADCTL `0x60`, software SPI mode 0, with no GPIO11 backlight control.
- GPS remains pinmap/logging only, not full NMEA parsing.
- SLE advertise TX power and scan-response declaration remain unified at `18 dBm`.

## Known Limits

- LVGL remains optional and depends on SDK header/library availability; the firmware can fall back to the built-in text renderer.
- This release fixes deployment/config control paths. It does not add a new mesh routing protocol feature.
