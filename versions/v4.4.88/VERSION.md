# Version v4.4.88

Date: 2026-06-05

## Scope

`v4.4.88` fixes the final topology convergence failure found after flashing and testing
`v4.4.87` on four boards.

Authoritative evidence:

```text
logs/live/v4.4.87_four_board_com16_leader_20260605_212712/leader_COM16.log
[team] route metrics active=3 direct=2 relayed=1 stale=0 unreachable=0 plan=1 converged=0 epoch=18
[team] route note member=86 conn=1 dir=2 next=224 new=0
[sle-rx] ROUTE_UPDATE 86->154 seq=40
[team] route note member=86 conn=1 dir=2 next=154 new=0
```

The earlier `v4.4.87` fix allowed control-packet bridging during relay rejoin, and that
part worked. The remaining failure was a route bookkeeping problem on the leader.

## Root Cause

On the leader, `team_bind_packet_source()` correctly preserved the physical first hop
for relayed packets, but then `ROUTE_UPDATE` parsing could overwrite that physical
first hop with the packet body's self-reported `next_hop_id`.

For a packet logically from member `86` but physically forwarded by relay `224`, the
leader briefly recorded `86 -> 224`. When `ROUTE_UPDATE 86->154` arrived through the
same relay path, the leader overwrote it as `86 -> 154`, so route metrics oscillated
between relayed and direct and never reached `direct=1 relayed=2 plan=0`.

## Fix

- Firmware visible version is bumped to `v4.4.88`.
- Leader-side route binding now keeps the physical first hop for relayed `ROUTE_UPDATE`
  packets.
- Direct `ROUTE_UPDATE` packets can still use their body next-hop normally.
- Added a log marker: `route update keep physical next_hop`.
- Tests and version guards are synchronized to `v4.4.88`.

## Verification

Pending at creation time:

- `git diff --check`
- Python `py_compile`
- Python unit tests
- Remote Ubuntu firmware build
- Parallel flash
- Four-board hardware validation
