# v4.4.76 Auto Flash And Test Flow

Date: 2026-06-05

## Successful Parallel Flash

Use this project wrapper instead of rebuilding burn commands by hand:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.76 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

Verified successful run:

```text
logs/burn/v4.4.76_20260605_155320/run_summary.txt
COM16: exit=0
COM13: exit=0
COM17: exit=0
COM18: exit=0
```

Key success evidence in each port log:

```text
Auto reset: sending CLI command 'reboot'
Establishing ymodem session...
Done. Reseting device...
```

Do not ask for manual reset for the normal v4.4.76 flow. The tested boards enter
the loader through the firmware serial `reboot` command.

## Four-Board Relay Test

Target topology:

```text
COM16: leader
COM13: member, relay candidate
COM17: member
COM18: member
leader direct cap: 1
```

Run:

```powershell
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.76 `
  --direct-cap 1 `
  --log-dir logs\live\v4.4.76_four_board_com16_leader_<timestamp>
```

Acceptance requires stable final topology plus:

```text
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```
