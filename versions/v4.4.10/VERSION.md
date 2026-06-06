# Version v4.4.10

Date: 2026-05-31

## Scope

`v4.4.10` focuses on 30-node stability and relay-failure recovery with the
systematic-debugging flow (root-cause first, then minimal fixes + regression).

## Root Cause (Member Reboot / Partial Offline Not Recovering)

Observed field symptom: after some members (or a hidden relay member) reboot or
drop, the leader could stay in `lost` state and not fully recover online count.

Root cause in leader loop:

- forced rescan was only triggered when:
  - pairing window is open, or
  - client connected count is zero.
- in a 30-node deployment, partial offline is common (not zero connections),
  so leader did not actively rescan for missing members.

## What Changed

1. Leader offline-member rescan gate added.
   - New helper: `team_leader_has_offline_members()`.
   - Leader now also triggers `team_leader_rescan_if_needed()` when offline
     members exist.
   - New explicit reason in log: `member_offline`.

2. SLE client pairing cache cleanup strengthened for reboot/rejoin loops.
   - Remove paired record before connect / when pair-none / on disconnect.
   - Pair-complete logs now include status name.

3. Capacity aligned to 30 logical members.
   - `SLE_TEAM_MAX_LOGICAL_MEMBERS` raised to `30U`.
   - Demo/simulation assertions and member loops updated to 30 nodes.

## Relay Suddenly Offline: What Happens Now

When a relay member suddenly drops:

1. Leader marks member offline via heartbeat timeout path.
2. `on_relay_offline` callback triggers immediate rebalance.
3. Offline relay permission is revoked (`offline`/`stale` path).
4. Leader promotes new relay candidates (RSSI + freshness policy).
5. Leaf members continue parent autoselect/reselection and report route update.
6. Leader periodic rescan now also covers partial-offline cases, helping missing
   nodes rejoin without requiring “all disconnected” state.

## Expected Outcome for 30 Nodes

- Partial member loss no longer blocks leader recovery logic.
- Hidden relay drop should converge by rebalance + member reselection, then
  online count recovers as nodes rejoin.
