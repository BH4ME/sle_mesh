# Manifest v4.4.94

Date: 2026-06-06

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_flash_team.sh`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.94/VERSION.md`
- `versions/v4.4.94/MANIFEST.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `.planning/2026-06-06-dynamic-relay-capacity/task_plan.md`
- `.planning/2026-06-06-dynamic-relay-capacity/findings.md`
- `.planning/2026-06-06-dynamic-relay-capacity/progress.md`

## Notes

This is a code and version-management change for stable relay role optimization. It does not change member table capacity, leader direct capacity, or the dynamic relay budget formula.

## Verification

- `npm --prefix webui test`
- `.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py`
- `.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -p test_ws63_auto_burn.py -t .`
- `.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -p test_ws63_four_board_relay_test.py -t .`
- Remote Ubuntu build through `automation\ws63\tools\ws63_remote_build_v4.py` against `owen@192.168.6.5:/home/owen/workspace/bearpi-pico_h3863`
- `scripts\ws63_flash_multi.ps1 -Ports COM16,COM13,COM17,COM18 -Parallel -Firmware output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg -ExpectedVersion v4.4.94 -ResetCommand reboot -ManualRetryTimeout 0`
- `.tooling\py311\python.exe automation\ws63\tools\ws63_four_board_relay_test.py --leader-port COM16 --relay-port COM13 --child1-port COM17 --child2-port COM18 --expected-fw v4.4.94 --direct-cap 1 --reboot-command "cfg reboot" --log-dir logs\live\v4.4.94_four_board_20260606_121503`
- `git diff --check`

All listed checks passed locally, remotely, or on the four attached WS63 boards; `git diff --check` reported CRLF normalization warnings only.

## Build Artifact

- Local package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Size: `1,604,456` bytes.
- SHA256: `3274B3AE49213589F1E8D6BFEDC238B04171B41A2462AD1A273466D4FF591543`
- Version guard: package contains `v4.4.94`.
- Relay swap guard: package contains `relay swap observe`, `swap-promote`, `swap-demote`, and `v4.4.94 relay swap hysteresis`.

## Hardware Evidence

- Burn logs: `logs\burn\v4.4.94_20260606_121042`
- Four-board logs: `logs\live\v4.4.94_four_board_20260606_121503`
- COM16 leader route id: 154, suffix: 279A.
- COM13 initial relay route id: 241, suffix: E7F1.
- COM17 child1 route id: 224, suffix: E7E0.
- COM18 child2 route id: 86, suffix: 5556.
- Direct-cap test evidence: `cfg direct 1` produced `relay_budget=1`; route metrics converged to `active=3 direct=1 relayed=2`.
- Member reboot evidence: child1 route 224 rebooted, restored member NV, rejoined, and was reported online by the leader.
- Relay reboot/failover evidence: original relay route 241 loss triggered immediate rebalance; route 86 was auto-promoted to relay.
- Original relay recovery policy: route 241 returned online as a normal member, while route 86 retained `relay=1`.
