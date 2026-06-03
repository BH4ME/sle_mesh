# Version v4.4.11

Date: 2026-06-01

## Scope

`v4.4.11` focuses on faster offline detection while keeping 30-node runtime
behavior stable.

## Root Cause

The observed 5-6s "lost" delay was mainly caused by layered timeout windows:

1. Link supervision timeout in SLE advertise params was ~5000 ms.
2. Business heartbeat timeout default was 10 s (`hb=3s, timeout=10s`).
3. Leader offline marking relied heavily on periodic timeout prune, not
   immediate disconnect path for known downstream member links.

## What Changed

1. Heartbeat policy moved to low-latency defaults:
   - `heartbeat_interval_s`: `3 -> 1`
   - `heartbeat_timeout_s`: `10 -> 4`

2. SLE link supervision timeout reduced:
   - `conn_supervision_timeout`: `0x1F4 (~5000ms) -> 0xFA (~2500ms)`
   - paired with near value `conn_max_latency`: `0xF9`

3. Leader now marks known member offline immediately on disconnect callback:
   - resolve member ID from connection mapping (`client/server conn -> member`)
   - fallback to tracked route ID when mapping is missing
   - clear relay capability and route-next-hop state for that member
   - trigger display/status refresh and downstream rescan for leader client path

4. Leader rescan cadence tightened for partial-offline recovery:
   - `SLE_TEAM_MEMBER_RESCAN_INTERVAL_S`: `12 -> 3`

5. Version bump:
   - firmware string/hardware tag: `v4.4.11`

## Expected Impact

- Faster "member lost" visibility (disconnect-driven + shorter timeout windows).
- Better practical recovery speed under partial offline conditions.
- 30-node role/relay model remains unchanged (logical max still 30).

## Notes

- Faster timeout policies may increase sensitivity under heavy RF interference.
- If field false positives rise, tune fallback to `hb=1s timeout=5s`.
