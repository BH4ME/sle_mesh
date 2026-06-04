# v4.4.48 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_flash_team.sh`
- `README.md`
- `versions/README.md`
- `versions/v4.4.48/VERSION.md`
- `versions/v4.4.48/MANIFEST.md`

## Key Deltas

1. Added an idempotent runtime config check for already-matching leader/member role config.
2. Kept true runtime role/config changes blocked while SLE is already configured.
3. Moved firmware-visible version strings and build/flash stale-image guards to `v4.4.48`.

## Verification

Completed on 2026-06-04 with local WSL build, COM13/COM16 live boards, and relay simulation:

```sh
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_link_cycle_test automation.ws63.tests.test_ws63_system_script
bash -lc "cc -Wall -Werror -I/mnt/e/codex_documents/sle/include /mnt/e/codex_documents/sle/examples/team_node_regression_test.c /mnt/e/codex_documents/sle/src/sle_team_packet.c /mnt/e/codex_documents/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
python automation/ws63/tools/ws63_auto_burn.py -p COM13 --software-reset-only --expected-version v4.4.48 output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
python automation/ws63/tools/ws63_auto_burn.py -p COM16 --software-reset-only --expected-version v4.4.48 output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
python automation/ws63/tools/ws63_link_cycle_test.py --leader-port COM16 --member-port COM13 --bootstrap-roles --team-id 1 --channel 17 --member-id 241 --state-timeout-s 45 --reboot-offline-timeout-s 30 --member-boot-timeout-s 60 --no-auto-rejoin-s 8 --log-dir logs/auto_test/v4.4.48_goal_live_confirm
bash scripts/simulate_v2.sh --suite=failover --stress=1 --py-members=30 --py-direct-cap=8 --py-relay-target=3 --py-fail-tick=6 --py-recover-tick=10 --py-ticks=16 --py-batch-fail-relay-count=2 --py-batch-fail-relay-ticks=8,12
bash -lc "mkdir -p logs/sim && python3 tools/sle_team_python_sim.py --members 30 --direct-cap 8 --relay-target 3 --relay-fail-tick 6 --relay-recover-tick 10 --ticks 16 --batch-fail-relay-count 2 --batch-fail-relay-ticks 8,12 --stress 5 --seed 20260604 --jitter-min-ms 0 --jitter-max-ms 80 > logs/sim/python_1v30_v4.4.48.log"
```

## Firmware Package

- Output: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Size: `1592360` bytes
- SHA256: `3DD0C925E07B2B9947758C92F760D3E7240CD11988CD5482BA3920CC8B22A74F`
- Package marker: `v4.4.48`
- ELF guards present: `v4.4.48`, `[display] st7789 ready`, `[team] boot unconfigured`, `[cfg-json]`

## Live Evidence

COM13 and COM16 were both flashed with `--expected-version v4.4.48`, so stale packages are rejected before burning. Both burns completed all YMODEM transfers at 100%.

Live COM13/COM16 test:

- Log directory: `logs/auto_test/v4.4.48_goal_live_confirm`
- Command result: `[link-cycle] PASS: reboot restore + manual leave/rejoin`
- Leader: COM16, route id `154`, suffix `279A`
- Member: COM13, route id `241`, suffix `E7F1`

Requirement 1 evidence, member reboot is link loss, not manual leave:

- Member log line 181: `[tx] reboot`
- Member log line 437: `[team-nv] restore member leader_suffix=279A leader=154 ret=0`
- Member log line 477: `[team] joined member=241`
- Leader log lines 363-381: heartbeat timeout, disconnect, seek restarted, `leader force rescan reason=member_offline`
- Leader log lines 431-439: `HELLO 241->154`, `CONFIG`, `ACK`, then `[team] joined member=241`

This proves that after saved flash config exists, member reboot does not require a serial `join`; serial was only used for initial role bootstrap.

Requirement 2 evidence, manual leave clears config and manual rejoin works:

- Member log line 491: `[tx] leave`
- Member log line 493: `[state] member left team`
- Member log line 495: `[team-nv] clear web config ret=0x0 flush=0x80000002`
- Leader log lines 475-479: `ALERT 241->154`, then `[team] member offline id=241 reason=member_leave`
- Member log lines 501-505: manual `role member 279A` saved again with `ret=0`
- Leader log lines 507-513: `HELLO 241->154`, `CONFIG`, `ACK`, then `[team] joined member=241`

Requirement 3 evidence, relay/30-node behavior:

- `bash scripts/simulate_v2.sh --suite=failover ... --py-members=30 ...` passed.
- `logs/sim/python_1v30_v4.4.48.log` ran 5 stress iterations with `discovered=30 approved=30`, `relay_reselect=5`, `batch_fail_events=2`, and `lost_parent=0`.
- `logs/sim/relay_rebalance_test.log`: `[relay-rebalance] pass: drop->promote->rejoin behavior verified`
- `logs/sim/failover_suite.log`: all failover scenarios passed.

Hardware limitation:

- Requirements 1 and 2 are live-verified on COM13/COM16.
- Requirement 3 is code/simulation-verified for 30 logical members, relay loss, relay reselection, reparenting, and relay recovery.
- Full live relay proof still requires at least three physical boards: leader, relay member, and child member.
