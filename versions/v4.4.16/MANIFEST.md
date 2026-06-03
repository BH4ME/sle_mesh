# v4.4.16 Manifest

## Changed Files (this iteration)

- `src/sle_team_node.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.16/VERSION.md`
- `versions/v4.4.16/MANIFEST.md`

## Key Logic Deltas

1. Allowlist rejoin deadlock fix:
   - In leader mode, empty allowlist (`allow=only` + `count=0`) is now treated
     as non-blocking, not deny-all.
   - Prevents false `member rejected by allowlist` after member power-cycle.

2. Firmware version bump:
   - `SLE_TEAM_FW_VERSION` and board tag moved to `v4.4.16`.

3. Version-management sync:
   - root/module/version index docs aligned to `v4.4.16`.

## Build / Verification

- Local unit/sim:
  - `python tools/test_sle_team_python_sim.py`
  - Note: run from repo root with module path available.

- Remote Ubuntu build:
  - Host: `192.168.6.5`
  - SDK: `/home/owen/workspace/bearpi-pico_h3863`
  - Command: `python3 build.py ws63-liteos-app -j4`
  - Output:
    - Remote: `/home/owen/workspace/bearpi-pico_h3863/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg`
    - Local: `E:\codex_documents\sle\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg`
