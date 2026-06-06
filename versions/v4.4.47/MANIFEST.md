# v4.4.47 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_flash_team.sh`
- `README.md`
- `versions/README.md`
- `versions/v4.4.47/VERSION.md`
- `versions/v4.4.47/MANIFEST.md`

## Key Deltas

1. Treat `uapi_nv_flush()` warnings as non-fatal after successful NV writes.
2. Keep flush return logging for future field diagnosis.
3. Move firmware-visible and burn-guard version strings to `v4.4.47`.

## Verification

Completed on 2026-06-04:

```sh
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_link_cycle_test automation.ws63.tests.test_ws63_system_script
bash -lc "cc -Wall -Werror -I/path/to/sle/include /path/to/sle/examples/team_node_regression_test.c /path/to/sle/src/sle_team_packet.c /path/to/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
python automation/ws63/tools/ws63_auto_burn.py -p COM13 --software-reset-only --expected-version v4.4.47 output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
python automation/ws63/tools/ws63_auto_burn.py -p COM16 --software-reset-only --expected-version v4.4.47 output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
python automation/ws63/tools/ws63_link_cycle_test.py --leader-port COM16 --member-port COM13 --bootstrap-roles --team-id 1 --channel 17 --member-id 241 --log-dir logs/auto_test/v4.4.47_live
```

Actual live-board evidence:

- Built local WSL package `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`, size `1592168`, containing `v4.4.47` and not containing `v4.4.46`/`v4.4.37`.
- Flashed `v4.4.47` to `COM13` and `COM16` with `--expected-version v4.4.47`; both burns exited `0`.
- `COM16` configured as leader, `COM13` configured as member. Both logged `uapi_nv_write ret=0x0` with `flush=0x80000002`, and both returned role config `ret=0`.
- Reboot-only test passed in `logs/auto_test/v4.4.47_reboot_only`: only `reboot` was sent to member, no `role member` or `join`; member logged `[team-nv] restore member leader_suffix=279A leader=154 ret=0`, and leader reported `member=241 online=1`.
- Full lifecycle test passed in `logs/auto_test/v4.4.47_live_full_cycle_fixed`: member reboot auto-restored, manual `leave` cleared config and did not auto-rejoin, manual rejoin succeeded.
