# v4.4.68 Flash And Four-Board Test

## Staggered Parallel Flash

Use this command for multi-board flashing:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM13,COM17,COM18 `
  -ExpectedVersion v4.4.66 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 15 `
  -ManualRetryTimeout 240
```

The script uses:

```text
software-reset-only
legacy-reset-order
no-reset-preamble
reset-command reboot
reset-command-fallback reset
parallel_start_delay_ms recorded in run_summary.txt
per-port exit codes recorded in run_summary.txt
```

Success still requires a separate `cfg status` confirmation for every board.

Current verified evidence:

```text
local regression: PASS, <repo-root>\logs\local\v4.4.68_20260605_094309\local_regression.log
staggered parallel flash: PASS, <repo-root>\logs\burn\v4.4.66_20260605_094335\run_summary.txt
post-flash status confirm COM13/COM17/COM18: PASS, <repo-root>\logs\serial\v4.4.68_staggered_parallel_confirm_20260605_094640\confirm.log
COM16 status confirm: FAIL, 0 bytes / no cfg-json
```

## Four-Board Test

Topology:

```text
COM16: leader
COM13: member approved as relay
COM17: member child
COM18: member child
leader direct cap: 1
```

Command after COM16 responds:

```powershell
$ts = Get-Date -Format "yyyyMMdd_HHmmss"
$logDir = "logs\live\v4.4.68_four_board_com16_leader_$ts"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.66 `
  --team-id 1 `
  --channel 17 `
  --direct-cap 1 `
  --reboot-command "cfg reboot" `
  --log-dir $logDir
```

Do not run this as an acceptance test until `COM16` returns `[cfg-json]` with `fw:"v4.4.66"`.
