# v4.4.92 Flash And Four-Board Test

Date: 2026-06-05

## Firmware Package

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
expected version guard: v4.4.92
```

## Parallel Flash Command

Use the project wrapper instead of rebuilding burn commands by hand:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.92 `
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

## Four-Board Test Command

```powershell
$ts = Get-Date -Format 'yyyyMMdd_HHmmss'
$logDir = "<repo-root>\logs\live\v4.4.92_four_board_com16_leader_$ts"
python .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.92 `
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

## Required Evidence

- COM16 reports `fw=v4.4.92` and `runtimeRole=leader`.
- COM13/COM17/COM18 report `fw=v4.4.92` and `runtimeRole=member`.
- Relay rebalance logs include `relay rebalance demand online=... known=... pending=... demand=... direct_cap=... relay=... target=...`.
- Leader keeps at least one relay target for the known four-board deployment when `direct-cap=1`.
- Child `HELLO child->leader` packets through COM13 are followed by `relay forwarded packet`, not `ret=-4` before forwarding.
- Final route metrics converge with `active=3`, `direct=1`, `relayed=2`, `stale=0`, `unreachable=0`, `converged=1`.

## Current Status

Code updated. Build, flash, and four-board hardware validation have not been run yet for v4.4.92.
