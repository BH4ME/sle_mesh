# Version v4.4.134

## Type

Firmware WS2812 status-light blink follow-up release.

## Firmware Version

The WS63 firmware version is now `v4.4.134`.

## Summary

- Advanced firmware and current repository records from `v4.4.133` to `v4.4.134`.
- Kept the v4.4.133 simple WS2812 blink states:
  - idle/unconfigured: white blink, 160 ms on every 1600 ms.
  - leader: green blink, 220 ms on every 1000 ms.
  - joined member: blue blink, 220 ms on every 1000 ms.
  - configured but not joined/error: red fast blink, 160 ms on every 360 ms.
- Added `team_ws2812_restart_base_phase()` so a short event flash returns to the base blink from its on phase instead of resuming in a possible off phase.
- Updated build, flash, and automation defaults to expect `v4.4.134`.

## Root Cause

- The breathing path was removed in v4.4.133, but the event flash overlay could finish while the unchanged base blink phase was already in its off window.
- That made the light look like it skipped or landed randomly after a join/lost/warn flash even though the base state itself was correct.

## Validation

- Source-level tests now assert that flash completion calls `team_ws2812_restart_base_phase(now_ms)` before `team_ws2812_render_base_state(now_ms)`.
- Build guards reject any remaining WS2812 breathing path and require the blink phase anchor plus flash-completion phase restart.
- The final firmware package should contain `v4.4.134` and should not contain stale `v4.4.133`.
