# Version v4.4.30

Date: 2026-06-02

## Scope

`v4.4.30` focuses on the root causes behind member rejoin duplication, relay
dual-role callback collisions, and unsafe SSAP callback dereferences.

## Root Cause Evidence

1. Leader member table reuse was keyed by `online != 0`, so an offline member
   could no longer match its old logical record during rejoin.
2. The same leader-side `member_id` could then be recreated into a different
   slot, which explains field symptoms like `on 1 off 2` and stale `lost`
   counters after a member reboot/rejoin.
3. SLE client callbacks dereferenced SDK pointers without checking `status` or
   `NULL`, so transient discovery/write failures could crash or corrupt state.
4. Relay dual-role startup registered announce callbacks and seek callbacks in
   two separate `sle_announce_seek_register_callbacks(...)` calls, which risks
   the later registration overwriting the earlier half of the callback table.
5. Relay-side stale RSSI recovery was still gated by `sle_uart_client_is_ready()`
   instead of actual connected ACL count, so a stale downstream link could miss
   cleanup when discovery-ready state was lost.

## What Changed

1. Member record reuse is now based on logical `member_id` ownership, not only
   current `online` state.
2. Rejoining a previously offline member reuses the original slot and marks it
   back online instead of allocating a duplicate record.
3. SSAP client callbacks now guard `NULL`/failure status before dereferencing
   service, structure, or write-result pointers.
4. Announce/seek callback registration now uses a merge path so relay dual-role
   startup preserves both announce and seek handlers in one shared table.
5. Relay RSSI recovery now runs when client connections exist, even if property
   discovery-ready state is temporarily lost.
6. Firmware-visible version strings and project docs are synchronized to
   `v4.4.30`.

## Known Limits

1. This iteration validates logic and local WebUI tests, but did not complete a
   WS63 remote Ubuntu compile in this turn because the current SSH auth path was
   unavailable.
2. On-device flashing and 30-board field verification still need a dedicated
   hardware run after firmware build output is refreshed.
