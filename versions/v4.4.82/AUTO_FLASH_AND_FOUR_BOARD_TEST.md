# v4.4.82 Auto Flash And Four-Board Test Flow

`v4.4.82` keeps the `v4.4.81` four-board flow, but the purpose of the next
hardware run is specifically to verify the route bookkeeping fix from the
`v4.4.74` and `v4.4.81` feedback.

## Firmware Package

Use this package after a successful `v4.4.82` build:

```text
E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

Before flashing, verify the package contains:

```text
expected version: v4.4.82
```

## Parallel Flash

Default four-board ports:

```text
COM16: leader target
COM13: relay candidate member
COM17: child member
COM18: child member
```

Run this from `E:\codex_documents\sle` only after the user asks to burn:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\scripts\ws63_flash_multi.ps1 `
  -Ports COM16,COM13,COM17,COM18 `
  -ExpectedVersion v4.4.82 `
  -Parallel `
  -ParallelStartDelayMs 1200 `
  -WaitTimeout 45 `
  -ManualRetryTimeout 0
```

The wrapper records one log and one command file per port under `logs\burn`.
The verified reset flow remains:

```text
software-reset-only
reset command: reboot
no RTS/DTR assumption
no manual reset fallback during automated runs
```

## Four-Board Configuration And Test

Run this from `E:\codex_documents\sle` after all boards report `v4.4.82`:

```powershell
$stamp=Get-Date -Format 'yyyyMMdd_HHmmss'
$logDir="logs\live\v4.4.82_four_board_com16_leader_$stamp"
.\.tooling\py311\python.exe .\automation\ws63\tools\ws63_four_board_relay_test.py `
  --leader-port COM16 `
  --relay-port COM13 `
  --child1-port COM17 `
  --child2-port COM18 `
  --expected-fw v4.4.82 `
  --direct-cap 1 `
  --log-dir $logDir
```

Expected converged route metric:

```text
route metrics active=3 direct=1 relayed=2 stale=0 unreachable=0 plan=0 converged=1
```

## Regression Gates

The run must fail if either pattern appears:

```text
route hint member=<id> parent=<relay> ret=-4
[sle-rx] <leader-bound packet> followed by dst=<leader> reason=NO_ROUTE
```

Additional evidence to inspect for this version:

```text
[team] route note member=...
[team] route reconcile member=...
```

These lines prove route entries are refreshed when online members reappear and
route metrics are no longer stuck behind stale bookkeeping.
