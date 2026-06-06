# Version v4.4.55

Date: 2026-06-04

## Scope

`v4.4.55` is the disconnect-to-display reliability patch on top of `v4.4.54`. It keeps the `LINK-MESH` LVGL panel and fixes the remaining field gap where a real SLE disconnect could reconnect successfully without first showing a stable `LOST Mxxxx` / `REJOIN Mxxxx` lifecycle on the leader screen.

## Root Cause

Hardware testing with `COM16` as leader and `COM13` as member reproduced a specific missing event path: the leader received `[Disconnected]`, but did not always print `member offline id=241` or drive the ST7789 `LOST` event. The disconnect callback could lose the member mapping at the SLE connection boundary because member identity was tied to a connection record that is cleared during disconnect handling.

## What Changed

1. Added a route-table fallback lookup for `conn_id -> member_id` before the SLE connection is cleared.
2. Preserved the packet-learned `track->route_id` during disconnect handling instead of overwriting it from the disconnect address.
3. Added a short diagnostic line, `[team] disconnect lookup ...`, so live testing can prove whether the leader found the member before marking it offline.
4. Kept display labels on the MAC-suffix path, so internal route `241` still displays as `ME7F1` when MAC data is available.
5. Firmware-visible version strings and build/flash guards moved to `v4.4.55`.

## Expected Behavior

- Manual member leave still shows `LEFT Mxxxx`.
- SLE disconnect / member power loss marks the leader record offline and shows `LOST Mxxxx`.
- Member return after offline state shows `REJOIN Mxxxx`.
- The label for the tested member remains `ME7F1`, not decimal-only `M241`.
- Serial/WebUI configuration remains unchanged.
