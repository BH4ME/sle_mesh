# Version v4.4.82

Date: 2026-06-05

## Scope

`v4.4.82` is a feedback-driven firmware logic fix for the four-board relay
recovery issue. No flashing is part of this version record unless the user
explicitly asks for a burn/test run.

## Feedback Used

`v4.4.74` live feedback showed the leader could receive member traffic but hit
route lookup failure while sending route hints:

```text
[sle-tx-fail] type=PACKET dst=86 ret=-4 reason=NO_ROUTE
[team] route hint member=86 parent=241 ret=-4
```

`v4.4.81` then showed a related state split. The leader member table continued
receiving heartbeats from all members, but route metrics stayed stale:

```text
[team] route metrics active=2 direct=1 relayed=1 stale=0 unreachable=0 plan=0 converged=1 epoch=9
[sle-rx] HEARTBEAT 224->154 ...
[sle-rx] HEARTBEAT 241->154 ...
[sle-rx] HEARTBEAT 86->154 ...
```

That means SLE receive and member liveness were working; the failing component
was leader route bookkeeping.

## Root Cause

Leader route state could diverge from the member table after relay loss,
direct-cap pruning, and relay rejoin:

- Route entries were not forcing route metrics to refresh immediately when a
  route was created, changed, or cleared.
- The leader did not reconcile online member records back into route entries
  before route metric calculation.
- Leader packet ingress must treat `app_packet.src_id` as the logical origin,
  not necessarily the physical first-hop peer when a relay forwards child
  packets. The physical first hop must come from trusted connection tracking.

## Fix

- Firmware visible version is bumped to `v4.4.82`.
- `team_route_note()` now marks leader route metrics dirty on new or changed
  route entries and logs route changes.
- Route clear paths now mark route metrics dirty when entries are removed.
- Leader route metrics now call `team_leader_reconcile_online_routes()` before
  counting route state, so online members with missing/unreachable route entries
  are repaired from the current physical connection table.
- Leader packet binding now preserves the physical first-hop peer and only
  promotes provisional route IDs when the packet source confirms the same peer.
- The four-board regression test expected firmware is updated to `v4.4.82` and
  has source guards for the route dirty/reconcile and physical-first-hop logic.

## Runtime Status

This version has not been flashed in this turn. The next hardware step, only
after explicit user request, is to build/burn `v4.4.82` and rerun the four-board
COM16/COM13/COM17/COM18 relay recovery test.
