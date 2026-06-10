# Version v4.4.136

## Type

Firmware WS2812 visibility release.

## Firmware Version

The WS63 firmware version is now `v4.4.136`.

## Summary

- Advanced firmware and current repository records from `v4.4.135` to `v4.4.136`.
- Doubled WS2812 color levels so idle, leader, member, warning, error, TX/RX, boot marker, and RGB test output are easier to see.
- Increased blink on-time windows so blinking boards are less likely to look dark when checked by eye.
- Preserved the simple blink-state logic without restoring breathing effects.

## WS2812 Values

- RGB test: `64,64,64`, 120 ms per color.
- Boot marker: `0,6,16`.
- Idle/unconfigured: white `16,16,16`, 500 ms on / 1600 ms period.
- Leader: green `0,16,0`, 500 ms on / 1000 ms period.
- Member: blue `0,0,16`, 500 ms on / 1000 ms period.
- Error/not joined: red `16,0,0`, 220 ms on / 360 ms period.
- Event flash: 140 ms on / 80 ms off, 4 pulses.
- Manual CLI `rgb red|green|blue|white` now uses `16` for single colors and `10,10,10` for white.

## Validation

- Source-level tests assert the new firmware version, doubled WS2812 color levels, longer blink-on windows, and no breathing path.
- Remote build guards require the final firmware image to contain `v4.4.136` and the updated WS2812 blink/brightness constants.
