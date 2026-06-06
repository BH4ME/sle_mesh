# v4.4.44 Manifest

## Changed Files

- `scripts/ws63_flash_team.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.44/VERSION.md`
- `versions/v4.4.44/MANIFEST.md`

## Key Deltas

1. Added `EXPECTED_FW_VERSION` guard to the team flash script.
2. Default guard value is `v4.4.44`.
3. Updated visible firmware/build guard strings to `v4.4.44`.

## Verification

Pending final run after this version bump:

```sh
python -m unittest automation.ws63.tests.test_ws63_link_cycle_test
bash -lc "cc -Wall -Werror -I/path/to/sle/include /path/to/sle/examples/team_node_regression_test.c /path/to/sle/src/sle_team_packet.c /path/to/sle/src/sle_team_node.c -o /tmp/team_node_regression_test && /tmp/team_node_regression_test"
bash scripts/simulate_v2.sh --suite=all --iterations=1
bash automation/ws63/scripts/ws63_test_system.sh --quick
npm --prefix webui test
npm --prefix webui run build
git diff --check
```

Live validation remains blocked until a real `v4.4.44` package is built. The currently available local package contains `v4.4.37` and should be refused by the new guard.
