# GPT-5.4 Review Record

- Date: 2026-05-06
- Reviewer: GPT-5.4
- Reviewed commit: `21382a4` (`Implement hierarchical relay routing`)
- Scope:
  - `src/sle_team_node.c`
  - `xc/ws63_team_network/src/ws63_team_network_app.c`
  - `include/sle_team_node.h`
  - `examples/team_network_demo.c`

## Findings

Status update:
- 2026-05-06 follow-up patch addressed both findings below in the working tree after commit `21382a4`.
- Validation rerun:
  - `cc -Wall -Wextra -Werror -Iinclude -DSLE_TEAM_NETWORK_TEST src/sle_team_packet.c src/sle_team_node.c examples/team_network_demo.c -o /tmp/sle_team_network_test && /tmp/sle_team_network_test`
  - `cc -Wall -Wextra -Werror -Iinclude -c src/sle_team_node.c -o /tmp/sle_team_node.o`
  - `cc -Wall -Wextra -Werror -Iinclude -c src/sle_team_web_api.c -o /tmp/sle_team_web_api.o`
  - `cc -Wall -Wextra -Werror -Iinclude -c src/sle_team_cli.c -o /tmp/sle_team_cli.o`

### Resolved in follow-up

- The upstream connection track now preserves a previously learned first-hop relay id when a leader-origin packet is forwarded across that same upstream link.
- Member rejoin / disconnect now disables active relay forwarding without erasing leader-granted `relay_allowed` permission, so approved relays retain fast-rejoin eligibility.
- Route lookup now treats `next_hop_id` as strict forwarding intent: if `next_hop` exists but its mapped link is inactive, send path fails with `NO_ROUTE` instead of falling back to stale `conn_id`.
- Member `ROUTE_UPDATE` next-hop reporting now prefers route-table upstream path (`leader_id -> upstream conn -> first-hop relay route_id`) before falling back to legacy `get_connect_id()` heuristics.

### 1. Indirect member can mis-learn leader as its parent instead of the first-hop relay

- File:
  - `xc/ws63_team_network/src/ws63_team_network_app.c`
- Areas:
  - `team_conn_track_note_packet()`
  - `team_bind_packet_source()`
  - `team_member_current_next_hop_id()`
- Why it matters:
  - For upstream packets received by a member, `team_bind_packet_source()` updates the connection track with `app_packet.src_id`.
  - When an ACK/CONFIG is forwarded from leader through a relay, the packet source is still the leader, so the connection track can be overwritten from the actual first-hop relay id to `leader_id`.
  - That makes `team_member_current_next_hop_id()` prefer the logical leader id over the physical first-hop relay, which can cause the member to send `ROUTE_UPDATE` with the wrong `next_hop_id`.
- Impact before fix:
  - Parent state may look healthy while the recorded parent is wrong.
  - Leader route refresh and future reselection logic can drift from the real first-hop topology.

### 2. Upstream disconnect currently clears leader-granted relay permission too aggressively

- File:
  - `src/sle_team_node.c`
  - `xc/ws63_team_network/src/ws63_team_network_app.c`
- Areas:
  - `sle_team_node_member_leave()`
  - `sle_team_member_rejoin()`
  - `team_connection_state_changed_cbk()`
- Why it matters:
  - `sle_team_node_member_leave()` and `sle_team_member_rejoin()` call `sle_team_node_disable_member_relay()`, which clears `relay_allowed` as well as `relay_enabled`.
  - The app uses `sle_team_node_member_leave()` during upstream disconnect handling, so a previously approved relay loses its cached relay grant before fast rejoin completes.
  - If the leader ACK arrives but the follow-up CONFIG is delayed or lost, the member rejoins with `relay_allowed == 0`, so downstream relay scanning stays disabled even though leader approval should still be valid.
- Impact before fix:
  - Fast rejoin is less durable than intended.
  - Approved relay nodes can silently come back as plain members after transient upstream churn unless CONFIG is re-delivered successfully.

## Residual test gap

- Host-side demo coverage is good for packet/state transitions, but there is still no full local compile of `xc/ws63_team_network/src/ws63_team_network_app.c` in this environment because vendor WS63 headers are unavailable here.
