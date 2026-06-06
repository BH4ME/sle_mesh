# Version v4.4.67

Date: 2026-06-05

## Scope

`v4.4.67` is an automation and field-test workflow version. It does not change firmware business logic from the verified `v4.4.66` firmware image.

The active test goal is:

1. Flash/configure four boards with `COM16` as leader.
2. Use `COM13`, `COM17`, and `COM18` as members.
3. Force leader direct capacity to one device so one member acts as relay and the other two route through relay.
4. Verify member reboot recovery.
5. Verify relay reboot recovery, child relay self-election, and what happens when the original relay returns.

## Firmware Baseline

```text
firmware version string: v4.4.66
firmware package: E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
```

## What Changed

1. `scripts/ws63_flash_multi.ps1` now defaults to the proven burn flow:
   `software-reset-only + legacy-reset-order + no-reset-preamble + reboot/reset fallback`.
2. `scripts/ws63_flash_multi.ps1 -Parallel` is the documented multi-device flash path.
3. `automation/ws63/tools/ws63_four_board_relay_test.py` now defaults to `expected-fw=v4.4.66` and `reboot-command="cfg reboot"`.
4. The version documentation records the exact COM16 blocker and the four-board validation plan.

## Required Verification

This workflow version is accepted only when logs prove:

1. Local automation tests pass.
2. Parallel or sequential flash command prints one prefixed log stream per port and stores one log file per port.
3. `COM16` reports `fw:"v4.4.66"` before it is used as leader.
4. Four-board test configures `COM16` as leader with direct cap `1`.
5. Leader sees all three members online after enrollment.
6. Child/member reboot produces offline and rejoin evidence.
7. Relay reboot produces offline evidence, child relay self-election, and final policy summary after original relay returns.

## Command Log Slots

```text
local regression log: E:\codex_documents\sle\logs\local\v4.4.67_20260605_092345\local_regression.log
parallel flash log: E:\codex_documents\sle\logs\burn\v4.4.66_20260605_085733\run_summary.txt
COM16 app/status probe log: E:\codex_documents\sle\logs\live\v4.4.67_four_board_com16_probe_20260605_084714\run.log
COM16 flash retry log: E:\codex_documents\sle\logs\burn\v4.4.66_20260605_084749\COM16.log
COM16 serial multi-baud probe log: E:\codex_documents\sle\logs\serial\v4.4.67_com16_probe_20260605_085414\com16_probe.log
COM16 DTR/RTS probe log: E:\codex_documents\sle\logs\serial\v4.4.67_com16_dtr_rts_probe_20260605_085547\com16_dtr_rts_probe.log
COM16 no-reset boot handshake log: E:\codex_documents\sle\logs\burn\v4.4.67_com16_noreset_20260605_085647\COM16_noreset.log
COM16 recovery probe log: E:\codex_documents\sle\logs\serial\v4.4.67_com16_recovery_probe_20260605_091540\com16_recovery_probe.log
post-parallel status confirm log: E:\codex_documents\sle\logs\serial\v4.4.67_parallel_confirm_20260605_090106\confirm.log
four-board test log: not run to completion because COM16 has no app/bootloader reply
COM16 blocker doc: E:\codex_documents\sle\versions\v4.4.67\COM16_BLOCKER.md
```

## Verification Result

```text
local regression: PASS, 36 unit tests OK
parallel flash COM13/COM17/COM18: PASS
post-parallel cfg status COM13: PASS, fw=v4.4.66, route=241, suffix=E7F1
post-parallel cfg status COM17: PASS, fw=v4.4.66, route=224, suffix=E7E0
post-parallel cfg status COM18: PASS, fw=v4.4.66, route=86, suffix=5556
COM16 cfg status as leader probe: FAIL, 0 bytes / no cfg-json
COM16 software-reset flash retry: FAIL, no boot handshake before manual retry timeout
COM16 multi-baud serial probe: FAIL, 0 bytes at 115200/230400/460800/921600/9600
COM16 DTR/RTS probe: FAIL, 0 bytes after all tested control-line sequences
COM16 no-reset boot handshake: FAIL, no boot handshake
COM16 recovery probe: FAIL, serial break + long DTR/RTS holds + reopen/listen attempts all returned 0 bytes
four-board COM16 leader test: NOT COMPLETE, blocked by COM16 no app/bootloader response
```

## Relay Recovery Policy From Code

The current firmware does not hard-code "original relay always wins" or "new child relay always wins".

The policy is target-based:

1. Leader computes a relay target from online count and `cfg direct` capacity in `team_leader_relay_target_for_online()`.
2. With direct cap `1` and three online members, normal target is `1` relay.
3. When a relay goes offline, `team_leader_failover_begin()` records children that were routed through the lost relay and triggers immediate rebalance.
4. During the failover grace window, `team_leader_relay_target_with_failover()` keeps at least one promoted relay alive so the child relay is not immediately demoted just because downstream children are temporarily hidden.
5. If relay count is below target, `team_leader_pick_best_relay_candidate()` promotes the best online non-relay candidate.
6. If relay count is above target after the original relay returns, `team_leader_pick_worst_active_relay()` chooses the weakest/stalest relay for `auto-demote`.

Therefore, when the original relay returns, the final result depends on live records/RSSI/age:

```text
original relay regained relay role; child relay was demoted
new child relay retained role; original relay returned as member
multiple relays online after recovery
no relay flag observed after recovery
```

The four-board test prints the observed policy string at runtime, but the live COM16-leader run is still blocked until COM16 responds.
