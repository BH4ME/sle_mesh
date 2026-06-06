# Version v4.4.54

Date: 2026-06-04

## Scope

`v4.4.54` is the display-polish release on top of `v4.4.53`. It keeps the corrected member identity logic and upgrades the ST7789/LVGL leader screen into a clearer link-status panel for field deployment.

## Root Cause Carried Forward

`v4.4.53` fixed the confusing `LOST M241` symptom. The decimal value came from the internal member id, which is derived from the final MAC byte. The display path now formats members through the route-label formatter, so a member is shown as `Mxxxx` from the MAC suffix whenever MAC data is available.

## What Changed

1. Upgraded the LVGL screen layout to a `LINK-MESH` status/event panel with a gradient background, status card, event card, accent rail, and event-color rail.
2. Kept all member lifecycle events visible on the leader screen: `JOIN`, `LEFT`, `TIMEOUT`, `LOST`, and `REJOIN`.
3. Added short event hints for faster field reading: `NODE ONLINE`, `MANUAL LEAVE`, `HEARTBEAT T/O`, `LINK LOST`, and `BACK ONLINE`.
4. Changed long event text to wrap inside the event card instead of clipping immediately.
5. Increased the local LVGL heap from `12KB` to `20KB` for the extra panel objects.
6. Firmware-visible version strings moved to `v4.4.54`.

## Expected Behavior

- Leader boot screen shows `SLE//BOOT LINK-MESH`.
- Status line keeps compact self/online/offline/event information.
- First join shows `JOIN Mxxxx`.
- Manual leave shows `LEFT Mxxxx`.
- Heartbeat timeout shows `TIMEOUT Mxxxx`.
- Link loss shows `LOST Mxxxx`.
- Return after offline/lost/timeout shows `REJOIN Mxxxx`.
- The display should not show decimal-only internal labels such as `M241` when MAC suffix data is available.

## Hardware Boundary

This version changes display presentation and LVGL heap sizing only. It does not intentionally change the SLE packet format, WiFi/WebUI routes, serial configuration commands, ST7789 pinmap, ST7789 geometry, or the unified leader/member runtime-role model.
