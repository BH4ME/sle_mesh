# Version v4.4.68

Date: 2026-06-05

## Scope

`v4.4.68` is a workflow and automation version. It keeps the verified firmware baseline at `v4.4.66` and improves the multi-board flashing process after the `v4.4.67` parallel retry showed `COM18` could miss boot handshake when all three boards started at exactly the same time.

## Firmware Baseline

```text
firmware version string: v4.4.66
firmware package: <repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## What Changed

1. `scripts/ws63_flash_multi.ps1` now has `-ParallelStartDelayMs`, default `500`.
2. Parallel flash jobs are still concurrent, but each port job starts slightly staggered to reduce simultaneous reset/boot-handshake collisions.
3. `run_summary.txt` records `parallel_start_delay_ms` plus per-port `exit=<code>` results.
4. COM16 blocker documentation remains authoritative in `COM16_BLOCKER.md`.

## Required Verification

This workflow version is accepted only when logs prove:

1. Local automation tests pass.
2. PowerShell parser accepts `scripts/ws63_flash_multi.ps1`.
3. Staggered parallel flashing writes per-port results to `run_summary.txt`.
4. `COM13`, `COM17`, and `COM18` still report `fw:"v4.4.66"` after flash/status confirmation.
5. `COM16` must still not be used as leader until it returns `[cfg-json]`.

## Command Log Slots

```text
local regression log: <repo-root>\logs\local\v4.4.68_20260605_094309\local_regression.log
staggered parallel flash log: <repo-root>\logs\burn\v4.4.66_20260605_094335\run_summary.txt
post-flash status confirm log: <repo-root>\logs\serial\v4.4.68_staggered_parallel_confirm_20260605_094640\confirm.log
COM16 blocker doc: <repo-root>\versions\v4.4.68\COM16_BLOCKER.md
```

## Verification Result

```text
local regression: PASS, 36 unit tests OK
PowerShell parse scripts/ws63_flash_multi.ps1: PASS
staggered parallel flash COM13: PASS
staggered parallel flash COM17: PASS
staggered parallel flash COM18: PASS
post-flash cfg status COM13: PASS, fw=v4.4.66, route=241, suffix=E7F1
post-flash cfg status COM17: PASS, fw=v4.4.66, route=224, suffix=E7E0
post-flash cfg status COM18: PASS, fw=v4.4.66, route=86, suffix=5556
COM16 cfg status: FAIL, 0 bytes / no cfg-json
four-board COM16 leader test: NOT COMPLETE, blocked by COM16 no app/bootloader response
```

## Relay Recovery Policy From Code

The relay recovery policy remains the `v4.4.66` target-based logic:

```text
direct cap 1 + three online members -> relay target 1
relay loss -> failover window preserves/promotes a child relay
original relay return -> leader keeps relay count at target
too many relays -> weakest/stalest active relay is auto-demoted
too few relays -> best online candidate is auto-promoted
```

So the final live policy can be:

```text
original relay regained relay role; child relay was demoted
new child relay retained role; original relay returned as member
multiple relays online after recovery
no relay flag observed after recovery
```

The live COM16-leader four-board run is still blocked until COM16 responds.
