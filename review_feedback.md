# Review Feedback: SLE Relay Routing Update

## Current question

User asked whether the current code already implements this intended model:

1. Leader manually approves a few initial members.
2. Approved members automatically become relay nodes.
3. New members can connect through those relays.
4. Relay forwards join/approve/config traffic, but leader remains the only authority that approves members.
5. The system can grow toward logical 1vs20 without leader directly holding 20 physical SLE links.

## Short answer

Not fully.

The current code has pieces of this model:

- Members can be configured with `relay_enabled`.
- Relay forwarding exists in `src/sle_team_node.c`.
- A relayed member's `HELLO` can be forwarded toward leader.
- Leader can still approve by member id.
- ACK/CONFIG from leader can be forwarded back through relay if route/connection binding has been learned.
- VM build script is back to unified runtime-role firmware.

But it is not yet the explicit "approve first, then promote to relay" design. Relay eligibility is currently local/bucket-based and can become active before the member is actually approved/joined.

## Required changes for the intended approve-to-relay model

### 1. Gate relay activation on leader approval / joined state

Files:

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `src/sle_team_node.c`

Current behavior:

- `cfg.relay_enabled` is set during `team_node_init()` based on bucket:
  - member role
  - `team_route_bucket_from_ids(self_id, leader_id) < 3`
- `team_relay_start_client_if_ready()` starts relay client when:
  - role is configured
  - SLE server is started
  - role is member
  - `relay_enabled != 0`
  - there is at least one upstream/server-side connection

It does **not** check whether this member has been approved by leader (`g_team_node.joined != 0`).

Risk:

An unapproved or not-yet-joined member can start acting as relay just because its bucket says it is relay-capable. That is not the desired trust model.

Requested fix:

- `team_relay_start_client_if_ready()` should require `g_team_node.joined != 0` before starting downstream relay-client scanning.
- If relay should only be granted by leader, do not let local bucket alone enable active relay behavior.
- Keep advertisement/server behavior as needed for leader discovery, but do not scan/connect downstream as a relay until approved.

Suggested condition:

```c
if (g_team_node.joined == 0U) {
    return;
}
```

inside `team_relay_start_client_if_ready()` before `sle_uart_client_init()`.

### 2. Add explicit leader-granted relay permission, or clearly define deterministic relay policy

Files:

- `include/sle_team_packet.h`
- `src/sle_team_packet.c`
- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`

Current behavior:

- Leader approval only adds the member to allowlist and sends CONFIG/ACK.
- CONFIG currently contains timing/distance/timeout fields only.
- There is no explicit "you are allowed to relay" grant from leader.
- A member decides relay eligibility locally from bucket.

Risk:

This does not match "leader approves a few, then they become relay" as a leader-controlled action. It is closer to "any joined or even configured bucket-eligible member may become relay."

Requested fix options:

Option A — explicit leader grant:

- Extend config or add a small control field/message with relay permission:
  - `relay_allowed`
  - `route_bucket` or `relay_tier`
  - optional `max_downstream`
- In `sle_team_node_pairing_approve()`, leader decides whether the approved member gets relay permission.
- Member only sets active `relay_enabled` after receiving this leader-granted config.

Option B — deterministic but approval-gated policy:

- Keep bucket-based relay eligibility.
- But enforce: bucket eligibility only takes effect after `joined == 1`.
- Document that relay permission is deterministic, not individually assigned by leader.

For the user's intended model, Option A is cleaner.

### 3. Logical 1vs20 is still blocked by `SLE_TEAM_MAX_MEMBERS 8U`

File: `include/sle_team_node.h`

Current state:

```c
#define SLE_TEAM_MAX_MEMBERS 8U
```

This controls:

- leader member records
- pending member records
- allowlist size

Risk:

Even if physical SLE connections stay <=8, leader still cannot track/approve 20 logical members because business-layer tables are capped at 8.

Requested fix:

- Split physical connection limits from logical team size.
- Example:

```c
#define SLE_TEAM_MAX_LOGICAL_MEMBERS 20U
#define SLE_TEAM_MAX_DIRECT_CONNECTIONS 8U
```

- Use logical max for:
  - `members[]`
  - `pending_members[]`
  - allowed member list, or a separate allowlist max
- Keep SLE client/server connection tables at 8 unless hardware/SDK proves more.

This is essential for "logical 1vs20".

### 4. Relay path should become explicit, not only send-all fallback

Files:

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `src/sle_team_node.c`

Current behavior:

- Leader sending to a member it does not directly know can fall back to `sle_uart_client_send_all()`.
- Relay may forward packets if it has learned the downstream member binding from prior traffic.

This can work for a simple demo, but it is not a robust 1vs20 route model.

Risk:

- ACK/CONFIG for an indirect member may be broadcast to all first-tier relays.
- Multiple relays may see traffic they should not need.
- Scaling gets noisy as member count grows.
- If the relay that forwarded HELLO is not the one that later has the downstream binding, return traffic may fail.

Requested fix:

- When a relay forwards `HELLO` from child member to leader, include or record next-hop/path information.
- At minimum maintain a route table:

```c
member_id -> next_hop_conn_id / direction / last_seen
```

- Leader should learn that indirect member `M10` is reachable via relay `M4` or via a first-hop connection.
- Relay should learn child member routes from packet `src_id`.
- Send ACK/CONFIG/POS/HEARTBEAT using route table before falling back to send-all.

A one-hop relay MVP can be:

```text
Leader -> Relay -> Member
```

with:

```text
leader route table: M10 -> relay M4
relay route table: M10 -> downstream conn id
```

### 5. Connection callback direction handling is improved but still has first-connection race risk

File: `xc/ws63_team_network/src/ws63_team_network_app.c`

Current improvements:

- There is now `g_team_conn_tracks[]`.
- Pair/RSSI callbacks use shared direction lookup.
- Disconnect clears the track.

Remaining risk:

`team_conn_guess_direction_from_addr()` currently identifies downstream mainly by:

```c
sle_uart_client_is_pending_remote_addr(addr)
```

This uses a single pending remote address from the client module. If SDK callback order is unusual, or if scan moves to another candidate before pair completes, the first downstream `PAIR_COMPLETE` can still be misclassified as upstream/server.

Requested fix:

- Add app-layer pending address table populated when `team_client_seek_filter()` accepts a candidate.
- Store:
  - addr
  - route_id
  - bucket
  - intended direction = downstream
- Callback direction should check:
  1. existing conn track
  2. pending addr table
  3. client/server conn table fallback
- Avoid relying on one global `g_sle_uart_remote_addr` as the only pending identity.

### 6. Direction classification should actually use bucket information when available

File: `xc/ws63_team_network/src/ws63_team_network_app.c`

Previous feedback asked for:

- `candidate_bucket < self_bucket` => upstream/server-side
- `candidate_bucket > self_bucket` => downstream/client-side

Current code parses route id from advertisement in `team_client_seek_filter()`, but the connection callback direction path does not really use the advertised route/bucket. It mostly uses pending remote addr and previously learned conn track.

Requested fix:

- When seek filter accepts a candidate, store candidate route id and bucket in a pending connection record.
- In connection callback classification, use that pending route/bucket to decide downstream.
- For upstream/server-side connections, use packet binding from `HELLO` as authoritative once received.

### 7. `sle_uart_server_adv.h` should be self-contained

File: `xc/ws63_team_network/sle_uart_server/sle_uart_server_adv.h`

Current risk:

The header uses:

```c
errcode_t
SLE_ADDR_LEN
```

but only includes `<stdint.h>`. It currently compiles only if callers happened to include other headers first.

Requested fix:

Add the required includes directly in the header, for example:

```c
#include "errcode.h"
#include "sle_common.h"
```

or whichever SDK header defines `SLE_ADDR_LEN` in this project.

### 8. VM build script usage should clarify leader/member aliases

File: `scripts/ws63_build_team_vm.sh`

Current state:

- Usage says only `unified`.
- Script still accepts `leader|member` as aliases, but they all build the unified image.

Requested fix:

Either:

- accept only `unified`, or
- document that `leader` and `member` are compatibility aliases and do not produce role-specific firmware.

## Status of previous review items

### Fixed or mostly fixed

- VM script now syncs local `include/`, `src/`, and `xc/ws63_team_network/` into VM SDK before building.
- VM script no longer toggles `CONFIG_SLE_TEAM_NODE_IS_LEADER` for separate leader/member firmware.
- `team_route_bucket_from_ids()` no longer subtracts leader id and therefore avoids unsigned wraparound.
- Advertising now includes a route hint and scan prefers that route id before falling back to SLE addr bytes.
- SLE client/server connection callbacks are centralized instead of overwriting each other.

### Still incomplete

- Relay activation is not approval-gated.
- Leader approval does not explicitly grant relay permission.
- Logical team size is still capped at 8, not 20.
- Return path routing still relies too much on send-all and learned conn binding rather than explicit route table.
- Connection callback first-event direction can still race without a pending addr/route table.

## Suggested tests for Codex to add/run

1. Approval-gated relay activation:
   - Member connects but is not approved.
   - Verify it does not start relay client scanning.
   - After leader approves it, verify relay client starts.

2. Indirect member approval:
   - Leader approves M2 directly.
   - M2 becomes relay only after approval.
   - M10 connects through M2.
   - M2 forwards M10 HELLO to leader.
   - Leader shows M10 pending.
   - Leader approves M10.
   - ACK/CONFIG returns through M2 to M10.

3. Logical member capacity:
   - Verify leader can store >8 pending/approved logical members without increasing physical SLE connection tables beyond 8.

4. Callback order robustness:
   - Simulate or instrument `PAIR_COMPLETE` before `CONNECTED` track allocation.
   - Verify downstream pair does not dispatch to server handler.

5. Route table behavior:
   - Verify ACK/CONFIG to indirect member uses the known relay/next hop, not only broadcast/send-all fallback.

6. Non-default leader id:
   - Configure leader id other than 1 and verify stable bucket assignment.

## One-line summary for Codex

The code has relay forwarding, but it is not yet the desired leader-controlled approve-to-relay model: relay activation is bucket/local and can happen before approval, logical member tables are still capped at 8, and route return paths need an explicit next-hop table rather than relying on send-all and learned bindings.

## Claude 审查补充

### Blocker
- `xc/ws63_team_network/src/ws63_team_network_app.c:2103-2126`
  `GET /api/pairing?action=approve` 里 `relay_allowed` 默认值是 `1`，不传 `relay` 参数时仍会默认允许中继。这会绕开“先批准，再显式授予 relay”的目标模型。

### Warning
- `xc/ws63_team_network/src/ws63_team_network_app.c:2467-2475, 2536-2544`
  `team_sle_send()` 在找不到精确下游路由时仍然回退到 `sle_uart_client_send_all()`。这会让多 relay 场景里同一包被多个第一跳同时收到，容易出现重复转发和不稳定投递。

### Note
- `xc/ws63_team_network/src/ws63_team_network_app.c:913-939, 2592-2618`
  路由表里虽然保存了 `next_hop_id`，但发送决策仍然主要按 `conn_id` 做，`next_hop_id` 没有真正参与选路。当前更像“学到一个连接就用它”，还不是显式 next-hop 路由。
