# Version v4.4.77

Date: 2026-06-05

## Scope

`v4.4.77` fixes the remaining four-board relay migration failure found after the
`v4.4.74` feedback chain.

Evidence reviewed:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/
logs/live/v4.4.75_four_board_com16_leader_20260605_144113/
logs/live/v4.4.76_four_board_com16_leader_20260605_155722/
```

`v4.4.74` exposed a leader NMI during the relay test. `v4.4.75` fixed the screen
flush/yield issue. `v4.4.76` fixed unsafe direct-link pruning by requiring
physical parent confirmation, but the live test still failed because children
kept rejoining the leader directly instead of the requested relay.

## Root Cause

When the leader direct-cap policy sent a `RESELECTING` route update, the child
correctly entered rediscovery with `upstream_parent_id = relay`.

However, two lower-level paths still pulled the child back to the leader:

1. `CONFIG` and `ACK` packets sent directly by the leader were allowed to mark
   the upstream parent as `CONNECTED`, overwriting the requested relay target.
2. The WS63 send path fell back to `send_report_by_handle()` when the requested
   relay connection was not ready, so HELLO traffic could still go over the old
   leader connection.

That produced the repeated log pattern in `v4.4.76`:

```text
child:  route update requests parent reselect
child:  upstream parent=154 state=2 reason=packet
leader: direct cap migrate member=86/224 parent=241
leader: route metrics active=3 direct=3 relayed=0 plan=2 converged=0
```

## Fix

- Firmware version is bumped to `v4.4.77`.
- Protocol core now keeps a non-leader reselect target active until that parent
  is physically used.
- Leader-direct `CONFIG` still updates configuration during reselect, but it no
  longer marks the leader as the connected upstream parent.
- Leader-direct `ACK` during reselect is deferred and does not mark the member
  joined.
- WS63 member send path no longer falls back to the leader/default handle while
  a specific relay reselect target is pending; it returns `RESELECT_PARENT`
  instead.
- WS63 seek filtering during reselect accepts only the requested relay route ID.
- `team_network_demo` now contains a regression for leader `CONFIG/ACK` not
  ending a relay reselect window.

## Required Verification

Before flashing boards, run:

```powershell
git diff --check

.\.tooling\py311\python.exe -m py_compile `
  automation\ws63\tools\ws63_four_board_relay_test.py `
  automation\ws63\tools\ws63_remote_build_v4.py `
  automation\ws63\tools\ws63_auto_burn.py

.\.tooling\py311\python.exe -m unittest `
  automation.ws63.tests.test_ws63_four_board_relay_test `
  automation.ws63.tests.test_ws63_auto_burn
```

Then build on `192.168.6.5`. Flash only after the code/build evidence passes.
