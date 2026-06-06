# v4.4.80 Auto Flash And Four-Board Test Flow

This file intentionally documents the next hardware step, but `v4.4.80` was
not flashed during the code-fix turn because the user asked to stop flashing
and fix the code from the `v4.4.74` feedback.

## Flash Command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.80 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

## Four-Board Test Command

```powershell
$stamp=Get-Date -Format 'yyyyMMdd_HHmmss'
$logDir="logs\live\v4.4.80_four_board_com16_leader_$stamp"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.80 `
  --direct-cap 1 `
  --log-dir $logDir
```

## Must Check In Logs

```text
No leader route hint/config failure caused by next_hop=leader/self direct route.
No repeated relay-side leader-bound HELLO NO_ROUTE.
Final leader route metrics converge:
active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```
