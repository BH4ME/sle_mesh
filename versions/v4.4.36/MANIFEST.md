# v4.4.36 Manifest

## Changed Files

- `.gitmodules`
- `README.md`
- `versions/README.md`
- `versions/v4.4.36/VERSION.md`
- `versions/v4.4.36/MANIFEST.md`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/src/api/client.ts`
- `webui/src/main.ts`
- `webui/src/protocol/types.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/third_party/lvgl-patches/lv8.3.11-ws63-c89-rect.patch`

## Key Logic Deltas

1. Persistent config writes now fail visibly when NV flush fails.
2. Leader quick config no longer silently hardcodes Team/Channel.
3. LVGL dependency is reproducible from a GitHub checkout through submodule metadata and a tracked patch.
4. Build and test guards are synchronized to `v4.4.36`.

## Verification

Pending in this working session before commit/push:

- WebUI contract tests and production build.
- Python 30-node relay failure/recovery simulations.
- Diff whitespace/integrity checks.
