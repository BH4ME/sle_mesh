# v4.4.79 Auto Flash And Four-Board Test Flow

Date: 2026-06-05

## Commands

Parallel flash:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.79 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

Four-board test:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.79 `
  --direct-cap 1 `
  --log-dir logs\live\v4.4.79_four_board_com16_leader_<timestamp>
```

## Topology

```text
COM16: leader
COM13: member, original relay candidate
COM17: member child / failover relay candidate
COM18: member child
leader direct cap: 1
```

## Acceptance

```text
enrollment PASS
initial route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
child reboot PASS
relay reboot failover observed
final route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
no repeated relay-side HELLO 241->154 NO_ROUTE after original relay returns
```

## Run Results

Pending build, flash, and live test.
