# v4.4.78 Auto Flash And Four-Board Test Flow

Date: 2026-06-05

## Purpose

This file records the repeatable v4.4.78 burn, configuration, and live
four-board relay test process. Use it before re-analyzing the flash flow.

## Parallel Flash Command

Use the project wrapper. Do not rebuild per-port burn commands by hand.

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.78 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

Expected package:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

Reset flow:

```text
software-reset-only + single serial CLI `reboot` command + post-package handshake
```

Normal v4.4.78 flashing should not require manual reset. Success evidence in
each port log:

```text
Auto reset: sending CLI command 'reboot'
Establishing ymodem session...
Done. Reseting device...
```

## Target Topology

```text
COM16: leader
COM13: member, relay candidate
COM17: member child
COM18: member child
leader direct cap: 1
```

With direct cap `1`, only one member remains directly attached to the leader.
The leader should auto-promote one member as relay, and the other two members
should route through that relay.

## Four-Board Test Command

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.78 `
  --direct-cap 1 `
  --log-dir logs\live\v4.4.78_four_board_com16_leader_<timestamp>
```

Acceptance requires the leader to report:

```text
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

## Runtime Behaviors To Verify

1. After enrollment, all three members are online.
2. Direct-cap routing converges to one direct member and two relayed members.
3. Rebooting one non-relay member produces a temporary leader-side loss/offline
   indication, then the member rejoins successfully.
4. Rebooting the relay produces leader-side relay loss evidence.
5. During relay loss, the two downstream members are not permanently dropped;
   one eligible child can be promoted as relay and routes recover.
6. When the original relay returns, the policy is target based:
   if only one relay is needed, the strongest/current best relay remains relay
   and the other relay-capable node is demoted to member.

## v4.4.78-Specific Log Evidence

The root-cause fix is physical, not only logical. Look for:

```text
child:  route update requests parent reselect
child:  parent reselect drop old leader conn=...
leader: direct cap migrate member=... parent=...
leader: direct cap prune confirmed member=...
leader: direct cap prune disconnect member=... no-offline
leader: route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

If the child logs `route update requests parent reselect` but never logs
`parent reselect drop old leader conn` or `parent reselect drop upstream`, the
physical convergence path is still broken and must be debugged before another
guess-fix.

## Run Results

Parallel flash completed:

```text
logs/burn/v4.4.78_20260605_164941/run_summary.txt
COM16: exit=0
COM13: exit=0
COM17: exit=0
COM18: exit=0
```

All four port logs include the expected automatic software-reset burn flow:

```text
Auto reset: sending CLI command 'reboot'
Establishing ymodem session...
Done. Reseting device...
```

Live four-board configuration and relay recovery test is in progress.
