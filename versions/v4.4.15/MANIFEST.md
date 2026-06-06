# v4.4.15 Manifest

## Changed Files (this iteration)

- `xc/ws63_team_network/third_party/lvgl/lv_conf.h`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.15/VERSION.md`
- `versions/v4.4.15/MANIFEST.md`

## Key Logic Deltas

1. LVGL compile stability under WS63 `-Werror`:
   - disabled unused LVGL widgets/extras/themes/layouts
   - kept label-focused rendering path for current screen UI

2. Firmware version bump:
   - `SLE_TEAM_FW_VERSION` and board tag moved to `v4.4.15`

3. Version-management sync:
   - root/module/version index docs aligned to `v4.4.15`

## Build / Verification

Remote Ubuntu build: PASS (Paramiko fallback path)

- Date: 2026-06-01
- Host: `192.168.6.5`
- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Command: `python3 build.py ws63-liteos-app -j4`
- Result: `Build target:ws63_liteos_app success`, package generated

Output package:

- Remote:
  `/home/owen/workspace/bearpi-pico_h3863/output/ws63/fwpkg/ws63-liteos-app/ws63-liteos-app_all.fwpkg`
- Local:
  `<repo-root>\output_from_vm\team_network_v4_unified_runtime_role\ws63-liteos-app_v4_unified_all.fwpkg`
- Local size: `1,588,776` bytes

Notes:

- Local environment lacked `sshpass`, so remote sync/build used Paramiko fallback
  per SOP.
