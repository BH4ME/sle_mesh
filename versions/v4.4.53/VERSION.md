# Version v4.4.53

Date: 2026-06-04

## Scope

`v4.4.53` is a firmware-visible display synchronization patch for the WS63 v4.4 line. It focuses on the leader ST7789/LVGL screen so member lifecycle events are understandable during deployment and debugging.

## Root Cause

The old screen alert rendered `LOST M%u` from the internal `member_id`. That ID is derived from the last MAC byte, so a member whose MAC suffix ends in `F1` appears as decimal `241`. This made the screen show `M241`, while WiFi/device labels use readable MAC suffixes such as `ME7F1`.

## What Changed

1. Replaced the old ST7789 alert path with a unified display event path: `JOIN`, `LEFT`, `TIMEOUT`, `LOST`, and `REJOIN`.
2. Event labels now use the existing route label formatter, so members display as `Mxxxx` from MAC suffix when available, for example `ME7F1`.
3. Leader display state now tracks whether a member is newly seen, already online, or previously offline, so duplicate `joined` callbacks do not spam the screen and a real return after offline shows `REJOIN`.
4. The screen status line now uses compact `O/F/E` counters for online, offline, and event count.
5. Firmware-visible version strings moved to `v4.4.53`.

## Expected Behavior

- First successful member join: `JOIN Mxxxx`.
- Manual member leave: `LEFT Mxxxx`.
- Heartbeat timeout: `TIMEOUT Mxxxx`.
- Connection/link loss: `LOST Mxxxx`.
- Member returns after any offline/lost/timeout state: `REJOIN Mxxxx`.
- The screen should not show `LOST M241` or other decimal-only member IDs when MAC suffix data is available.

## Hardware Boundary

This version changes display formatting and event tracking. It does not intentionally change SLE packet format, WiFi/WebUI routes, ST7789 pinmap, display geometry, or the existing unified leader/member runtime-role model.
