# Version v4.4.58

Date: 2026-06-04

## Scope

`v4.4.58` fixes the four-board relay enrollment path found during live testing.
The root cause was that a relay in `relay_discovery_only` mode forwarded `HELLO` and `ROUTE_UPDATE`, but blocked leader-to-child `CONFIG` and `ACK` packets. That made the leader believe a downstream member had been approved while the member never received the final join control plane.

## What Changed

1. Added a single discovery-only allowlist for relay control-plane packets: `HELLO`, `ROUTE_UPDATE`, `CONFIG`, and `ACK`.
2. Kept business traffic blocked while discovery-only is active, including heartbeat and position broadcasts.
3. Added a C protocol regression that proves `CONFIG` and `ACK` can pass leader -> relay -> child while discovery-only is enabled.
4. Bumped firmware-visible strings, build guards, WebUI contract tests, and flash guard defaults to `v4.4.58`.
5. Added `scripts/ws63_flash_multi.ps1` so multi-board flashing has visible per-port logs and optional parallel execution.

## Expected Behavior

- `COM16` can be configured as leader.
- `COM13`, `COM17`, and `COM18` can be configured as members.
- A bucket-1 member such as route id `241` can be approved as relay.
- Downstream members such as route ids `224` and `86` can receive `CONFIG/ACK` through that relay and complete join.
- During pairing/discovery-only relay mode, route control packets still flow, but normal business payloads stay blocked until pairing closes.

## Known Hardware Notes

- Boards without reliable RTS/DTR reset may require manual `BOOT+RESET` or `RESET/RST` when the burn log prints `Auto handshake timeout`.
- Parallel flashing is supported by script, but sequential flashing is safer when one person must manually press buttons on multiple boards.
