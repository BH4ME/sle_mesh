# v4.4.55 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.55/VERSION.md`
- `versions/v4.4.55/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

## Key Deltas

1. Fixed the leader disconnect path so `conn_id` can be mapped back to the member before SLE client/server connection records are cleared.
2. Added `team_route_member_by_conn()` as a route-table fallback for disconnect events.
3. Changed `team_conn_track_update()` so packet-learned route identity is not overwritten by a disconnect address once known.
4. Added `[team] disconnect lookup ...` instrumentation for hardware proof.
5. Updated version guards to `v4.4.55`.

## Verification

Pending final run on 2026-06-04:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn
git diff --check -- README.md versions/README.md versions/v4.4.55/VERSION.md versions/v4.4.55/MANIFEST.md webui/tests/ws63-api-contract.test.mjs xc/ws63_team_network/src/ws63_team_network_app.c scripts/ws63_build_v4_ubuntu.sh scripts/ws63_build_v4_local_wsl.sh scripts/ws63_flash_team.sh automation/ws63/tools/ws63_auto_burn.py automation/ws63/tests/test_ws63_auto_burn.py
```

Remote Ubuntu build target:

```sh
UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS=<local secret> UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 scripts/ws63_build_v4_ubuntu.sh unified
```

Live COM13/COM16 verification remains pending until the new package is built and flashed.
