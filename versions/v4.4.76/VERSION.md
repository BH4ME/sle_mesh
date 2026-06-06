# Version v4.4.76

Date: 2026-06-05

## Scope

`v4.4.76` fixes the four-board relay recovery failure observed after the
`v4.4.75` run. This is a firmware logic fix, not a flashing change.

Authoritative feedback log:

```text
logs/live/v4.4.75_four_board_com16_leader_20260605_144113/leader_COM16.log
logs/live/v4.4.75_four_board_com16_leader_20260605_144113/relay_COM13.log
```

## Root Cause

The leader accepted a logical `ROUTE_UPDATE` as proof that a member had already
migrated behind the relay, then pruned the old direct link immediately.

The failure sequence was:

```text
leader: ROUTE_UPDATE 154->241 seq=85
leader: direct cap prune member=241 conn=2 parent=224 cap=1
COM13:  ROUTE_UPDATE 154->241 seq=85
COM13:  Disconnected
```

That ordering was unsafe. A packet can still arrive on the old direct
connection while declaring `next_hop=relay`; this must not count as physical
reachability through the relay.

## Fix

- Firmware version is bumped to `v4.4.76`.
- Leader direct-cap pruning now requires both logical and physical confirmation:
  the route must match the desired parent and the packet must arrive through
  the desired relay connection.
- Member handling of leader `RESELECTING` route updates now enters rediscovery:
  `joined=0`, state `DISCOVERING`, and HELLO/config/heartbeat timers reset.
- Relay failover recovery also waits for the physical parent check before
  clearing the failover watch.
- Four-board test acceptance requires stable final topology plus converged route
  metrics, so a transient all-online state no longer passes.
- Default burn/build version guards now expect `v4.4.76`.

## Required Verification

No board flash is implied by this record. Before flashing, run:

```powershell
.\.tooling\py311\python.exe -m py_compile `
  automation\ws63\tools\ws63_four_board_relay_test.py `
  automation\ws63\tools\ws63_remote_build_v4.py `
  automation\ws63\tools\ws63_auto_burn.py

.\.tooling\py311\python.exe -m unittest `
  automation.ws63.tests.test_ws63_four_board_relay_test `
  automation.ws63.tests.test_ws63_auto_burn
```

Then build on `192.168.6.5`. Flash only after the user explicitly asks for it.
