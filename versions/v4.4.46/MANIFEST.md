# v4.4.46 Manifest

## Changed Files

- `automation/ws63/scripts/ws63_test_system.sh`
- `automation/ws63/tests/test_ws63_system_script.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.46/VERSION.md`
- `versions/v4.4.46/MANIFEST.md`

## Key Deltas

1. Fixed the automated burn-stage build command to use the v4 unified firmware builder.
2. Added a test guard so the burn path cannot silently regress to the old builder.
3. Synchronized visible firmware/build/flash guard strings to `v4.4.46`.

## Verification

Pending final run after this version bump:

```sh
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_link_cycle_test automation.ws63.tests.test_ws63_system_script
python automation\ws63\tools\ws63_auto_burn.py -p COM13 --no-auto-reset --expected-version v4.4.46 output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg
bash scripts/ws63_flash_team.sh --yes unified /dev/null
bash -lc "cc -Wall -Werror -I/path/to/sle/include /path/to/sle/examples/team_node_regression_test.c /path/to/sle/src/sle_team_packet.c /path/to/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Live validation remains blocked until a real `v4.4.46` package is built. The currently available local package contains `v4.4.37` and should be refused by both burn entry points.
