# v4.4.90 Flash And Four-Board Test

Date: 2026-06-05

## Firmware Package

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
expected version guard: v4.4.90
package size before flash: 1603176
```

## Parallel Flash Command

Use the project wrapper instead of rebuilding burn commands by hand:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.90 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

Expected success evidence for every port:

```text
Establishing ymodem session...
Done. Reseting device...
<PORT>: exit=0
result: PASS
```

The flash script writes:

```text
logs/burn/v4.4.90_<timestamp>/run_summary.txt
logs/burn/v4.4.90_<timestamp>/<PORT>.log
logs/burn/v4.4.90_<timestamp>/<PORT>.command.txt
```

## Four-Board Role Plan

```text
COM16: leader
COM13: member, original relay candidate
COM17: member child
COM18: member child
team: 1
channel: 17
leader direct capacity: 1
```

`direct-cap=1` forces only one direct member path. The remaining members should converge through a relay path, allowing relay reboot/failover validation.

## Four-Board Test Command

```powershell
$ts = Get-Date -Format 'yyyyMMdd_HHmmss'
$logDir = "<repo-root>\logs\live\v4.4.90_four_board_com16_leader_$ts"
python .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.90 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --initial-drain-s 2 `
  --cmd-timeout-s 25 `
  --state-timeout-s 90 `
  --route-timeout-s 120 `
  --offline-timeout-s 20 `
  --boot-timeout-s 75 `
  --failover-timeout-s 120 `
  --poll-interval-s 1 `
  --log-dir $logDir
```

The test defaults to clean-start:

```text
cfg clear on all boards
cfg reboot on all boards
verify runtimeConfigured=false
configure leader/member roles
set leader cfg direct 1
pair/approve COM13 as relay
pair/approve COM17/COM18 as non-relay members
```

## Required Evidence

- COM16 reports `fw=v4.4.90` and `runtimeRole=leader`.
- COM13/COM17/COM18 report `fw=v4.4.90` and `runtimeRole=member`.
- Leader member table shows three online members after enrollment.
- Child member reboot is observed by the leader as HELLO/rejoin and returns online.
- Original relay reboot is observed by the leader as offline/lost.
- One child becomes relay during original relay loss.
- When original relay returns, final policy is recorded explicitly: original relay regains relay role or returns as member while the child relay remains.
- Final route metrics converge with `active=3`, `direct=1`, `relayed=2`, `stale=0`, `unreachable=0`, `converged=1`.

## Current Status

Not run yet for v4.4.90.
