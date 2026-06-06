# v4.4.58 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.58/VERSION.md`
- `versions/v4.4.58/MANIFEST.md`
- `versions/v4.4.58/FLASH_AND_RELAY_TEST.md`
- `src/sle_team_node.c`
- `examples/team_network_demo.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_flash_multi.ps1`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `webui/tests/ws63-api-contract.test.mjs`

## Verification Plan

Planned for this version:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_link_cycle_test automation.ws63.tests.test_ws63_relay_cycle_test automation.ws63.tests.test_ws63_serial_preflight automation.ws63.tests.test_ws63_system_script
python -m tools.test_sle_team_python_sim
scripts/simulate_v2.sh --suite=core
git diff --check
```

Remote Ubuntu build target:

```sh
UBUNTU_HOST=192.168.6.5 \
UBUNTU_USER=owen \
UBUNTU_PASS='<local secret>' \
UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 \
BUILD_JOBS=4 \
scripts/ws63_build_v4_ubuntu.sh unified
```

Expected package:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.58` before flashing.

## Live Test Status

Observed after build and burn:

- Flash `COM16`, `COM13`, `COM17`, and `COM18`: passed.
- `cfg status` on all four boards: `fw=v4.4.58`, NV config writable.
- Configure `COM16` as leader and the other three boards as members: passed.
- Live join blocked before approval: leader scanned `COM13/E7F1` and printed
  `will connect`, but no connection callback followed. Leader stayed
  `seek active conn:0`, and members stayed `joined=0 / NOT_READY`.
- Failure logs:
  - `logs/live/v4.4.58_four_board_join_20260604_194925/`
  - `logs/live/v4.4.58_post_fail_probe_20260604_200013/`

This led to `v4.4.59`, which adds SLE seek-stop connect recovery.
