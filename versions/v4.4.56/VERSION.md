# Version v4.4.56

Date: 2026-06-04

## Scope

`v4.4.56` is the idempotent disconnect display patch on top of `v4.4.55`.
It keeps the `LINK-MESH` LVGL panel and fixes the remaining live-board gap where the leader could identify a disconnected member but still fail to print/display the `conn_disconnected` `LOST Mxxxx` event.

## Root Cause

`v4.4.55` proved that the leader can recover `conn_id -> member_id` during a real SLE disconnect:

```text
[team] disconnect lookup conn=0 dir=2 known=1 member=241 tracked=241 routed=241
```

The remaining failure was downstream of identity lookup. `team_leader_mark_member_offline()` returned early when the member record was already `online=0`, so if the core heartbeat/offline path consumed the state transition first, the later `conn_disconnected` event did not produce a stable `[team] member offline ... reason=conn_disconnected` log or `LOST Mxxxx` screen event.

## What Changed

1. Added per-member `display_member_last_events[]` so display logic can distinguish `LEFT`, `TIMEOUT`, `LOST`, and `REJOIN` history.
2. Made `conn_disconnected` handling idempotent: when the member is already offline, it still emits `LOST Mxxxx` unless the last display event was manual `LEFT`.
3. Kept manual leave protected from duplicate disconnect noise by suppressing `conn_disconnected` after `LEFT`.
4. Avoided duplicate relay-offline side effects when a disconnected member was already offline.
5. Updated firmware-visible version strings and build/flash guards to `v4.4.56`.

## Expected Behavior

- Manual member leave shows `LEFT Mxxxx` and is not overwritten by a later disconnect callback.
- Member reboot / SLE link loss shows `LOST Mxxxx` even if heartbeat timeout already marked the record offline.
- Member return after offline state shows `REJOIN Mxxxx`.
- Tested member route `241` still displays as MAC suffix label `ME7F1` when MAC data is available.
