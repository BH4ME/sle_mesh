# Version v4.4.132

## Type

Firmware WS2812 status-light simplification release.

## Firmware Version

The WS63 firmware version is now `v4.4.132`.

## Summary

- Advanced firmware and current repository records from `v4.4.131` to `v4.4.132`.
- Removed the WS2812 breathing animation path for the steady network states.
- Replaced the network-state RGB output with simple blink patterns:
  - idle/unconfigured: white blink, 160 ms on every 1600 ms.
  - leader: green blink, 220 ms on every 1000 ms.
  - joined member: blue blink, 220 ms on every 1000 ms.
  - configured but not joined/error: red fast blink, 160 ms on every 360 ms.
- Kept the existing short event flash overlay for join, leave, lost, warn, TX/RX-style events.
- Updated build, flash, and automation defaults to expect `v4.4.132`.

## Root Cause

- The previous breathing states shared a low base brightness and a dimming path that made LED behavior hard to read in the field.
- Because idle, leader, member, and error all shared the same breathing machinery, a confusing animation issue affected every steady status state at once.

## Validation

- Source-level WS2812 checks assert that `BREATHE` code is removed and the blink-state helpers are used by the base-state renderer.
- Four-board unit tests pass after updating the route-bind assertion to match the current bind-success guard.
- Build scripts guard the generated package for the `v4.4.132` marker before accepting the artifact.
