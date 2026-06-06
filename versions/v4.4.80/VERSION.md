# Version v4.4.80

Date: 2026-06-05

## Scope

`v4.4.80` fixes a route lookup bug found by re-reading the `v4.4.74`
four-board feedback instead of continuing to flash boards.

Evidence:

```text
live logs: logs/live/v4.4.74_four_board_com16_leader_20260605_135942/
leader_COM16.log: route hint member=86 parent=241 ret=-4
leader_COM16.log: sle-tx-fail type=PACKET dst=86 ret=-4 reason=NO_ROUTE
```

The key point from the log is that route hints/config packets could fail even
when the member was physically direct-connected, because the leader treated
`next_hop=leader/self` as if it needed to find a downstream member connection
for the leader id.

## Root Cause

`team_route_find()` treated any non-zero `next_hop_id` as a relay next-hop.
For a direct member route, a member can report `next_hop_id` as the leader id
or the local leader self id. The leader then tried to resolve a connection by
that next-hop id instead of using the route's recorded physical `conn_id`.

That turns a valid direct route into `NO_ROUTE`, which breaks config and
route-hint delivery during direct-cap migration and relay recovery.

## Fix

- Firmware version is bumped to `v4.4.80`.
- Added `team_route_next_hop_is_direct_peer()`.
- `team_route_find()` now treats `0`, broadcast, the member id, leader id, and
  local self id as direct next-hop semantics and uses the recorded physical
  `conn_id`.
- Route metrics and parent matching use the same direct/relay interpretation.
- Build and unit-test guards now require the direct next-hop source marker.

## Runtime Status

This version has not been flashed in this turn. That is intentional: the user
asked to stop flashing and fix the code based on the earlier `v4.4.74`
feedback first.

After local tests and remote Ubuntu build pass, the next hardware step is to
flash `v4.4.80` and rerun the existing four-board COM16 leader test.
