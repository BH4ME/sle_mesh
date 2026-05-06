# Findings & Decisions

## Requirements
- User wants to know whether this project can support 1vs20.
- User wants automatic routing behavior, not just manual point-to-point messaging.
- User is open to alternatives such as pseudo 1vs20, polling/scheduling, and layered routing.
- Any path should be grounded in the current codebase and WS63/SLE constraints, not just protocol theory.

## Research Findings
- The current repo explicitly states it is still a star topology rather than full mesh in [README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/README.md:104).
- The repo explicitly states the current member cap is still based on 8 connection slots rather than 20 in [README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/README.md:105).
- Core business-layer member state is fixed at 8 with `SLE_TEAM_MAX_MEMBERS 8U` in [include/sle_team_node.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_node.h:13).
- Leader/client connection management is fixed at 8 with `SLE_UART_CLIENT_MAX_CON 8` in [xc/ws63_team_network/sle_uart_client/sle_uart_client.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/sle_uart_client/sle_uart_client.c:33).
- Server-side connection bookkeeping is also fixed at 8 with `SLE_UART_SERVER_MAX_CONNECTIONS 8` in [xc/ws63_team_network/sle_uart_server/sle_uart_server.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/sle_uart_server/sle_uart_server.c:24).
- Field notes already anticipate an "8+ members strategy" and explicitly mention falling back to polling if SDK connection limits are not enough in [docs/field-notes-2026-05-04.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/field-notes-2026-05-04.md:272).
- The packet layer already contains route-related scaffolding: `route_type`, `hop_count`, and `ttl` exist in [include/sle_team_packet.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_packet.h:71) and [src/sle_team_packet.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_packet.c:198), even though the current runtime behavior is still star-shaped.
- Current send behavior is based on active connection IDs rather than free-air broadcast routing, so automatic routing would need explicit forwarding logic at the application layer.

## Technical Decisions
| Decision | Rationale |
|----------|-----------|
| Favor hierarchical routing over immediate true 1vs20 concurrent links | It aligns with the user goal of automatic routing while staying closer to the proven 1vs8 transport substrate. |
| Keep pseudo 1vs20 polling as backup architecture | If SDK or coexistence limits block 20 simultaneous links, polling still delivers a 20-node logical system. |
| Treat route metadata already in packets as a future extension point | Existing `ttl` and route fields reduce protocol churn if we add forwarding. |
| Split relay capability from relay activation | Latest review shows `relay_enabled` is currently local/bucket-derived, so unapproved members may relay before leader approval. The fix is to let leader CONFIG explicitly grant active relay permission. |
| Keep physical SLE connection tables at 8 while expanding logical team records to 20 | `sle_uart_client/server` remain SDK/transport-limited at 8, but business-layer `members`, `pending_members`, and allowlist need 20 for logical 1vs20. |
| Model upstream parent as explicit state plus route announcement | The app already tracks connection direction and next-hop routes, but the disconnect/reselect path needs a durable parent snapshot and an explicit `ROUTE_UPDATE` signal. |
| Preserve leader fast rejoin as allowlist-based, not manual approval-based | Approved members are already accepted again by allowlist; the missing piece is refreshing route state when the upstream path changes. |

## Issues Encountered
| Issue | Resolution |
|-------|------------|
| The installed GitHub skill names did not exactly match my initial guesses | Queried repo contents and installed the valid skill directories one by one. |
| `relay_enabled` conflates route capability and leader authorization | Leader CONFIG now carries `relay_allowed`, and member relay stays disabled until approval/join. |
| Parent reselection was not a first-class state machine | Upstream parent state is now explicit, and reconnect/timeout paths emit a fresh route update. |
| HTTP approve defaulting relay to true is a policy bypass | The API now defaults to no-relay unless the caller explicitly asks for relay authorization. |
| Fallback `send_all()` makes relay routing noisy | Unicast routing now fails closed when no next hop is known, rather than flooding every peer. |
| `next_hop_id` is only useful if it participates in send resolution | Route lookup now resolves the next-hop member first, then turns that into a concrete connection. |

## Resources
- [README.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/README.md)
- [docs/field-notes-2026-05-04.md](/Users/bh4me_macair/Documents/Codex/sle_intercom/docs/field-notes-2026-05-04.md)
- [include/sle_team_node.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_node.h)
- [include/sle_team_packet.h](/Users/bh4me_macair/Documents/Codex/sle_intercom/include/sle_team_packet.h)
- [src/sle_team_packet.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/src/sle_team_packet.c)
- [xc/ws63_team_network/sle_uart_client/sle_uart_client.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/sle_uart_client/sle_uart_client.c)
- [xc/ws63_team_network/sle_uart_server/sle_uart_server.c](/Users/bh4me_macair/Documents/Codex/sle_intercom/xc/ws63_team_network/sle_uart_server/sle_uart_server.c)

## Visual/Browser Findings
- No browser-derived findings were required for this phase; current conclusions come from local repo and local SDK inspection.
