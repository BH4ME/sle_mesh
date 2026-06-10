# Version v4.4.133

## Type

Firmware WS2812 status-light blink release.

## Firmware Version

The WS63 firmware version is now `v4.4.133`.

## Summary

- Advanced firmware and current repository records from `v4.4.132` to `v4.4.133`.
- Removed the steady-state WS2812 breathing animation path.
- Replaced steady RGB status output with simple blink states:
  - idle/unconfigured: white blink, 160 ms on every 1600 ms.
  - leader: green blink, 220 ms on every 1000 ms.
  - joined member: blue blink, 220 ms on every 1000 ms.
  - configured but not joined/error: red fast blink, 160 ms on every 360 ms.
- Anchored blink phase to the time a base state is entered so a role/status change starts visibly on, instead of landing randomly in an off phase.
- Kept the existing short event flash overlay for join, leave, lost, warn, TX/RX-style events.
- Updated build, flash, and automation defaults to expect `v4.4.133`.

## Root Cause

- The previous breathing states shared a low-brightness dimming path that was hard to read in the field.
- The first blink rewrite used absolute time for the blink phase, which could make a newly entered state start in the off portion of the cycle.

## Validation

- Source-level WS2812 checks assert that `BREATHE` code is removed, blink-state helpers exist, and the renderer uses `ws2812_base_enter_ms` as the blink phase anchor.
- Build guards reject any remaining WS2812 breathing path and require the `v4.4.133` marker.
- Four-board and auto-burn unit tests should pass before flashing.
