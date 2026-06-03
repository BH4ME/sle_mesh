# v4.4.31 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/Kconfig`
- `scripts/ws63_build_v4_ubuntu.sh`
- `webui/src/api/client.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.31/VERSION.md`
- `versions/v4.4.31/MANIFEST.md`

## Key Logic Deltas

1. Board SoftAP/HTTP WebUI auto-start is now a default-on firmware and Kconfig
   path, because the hosted/domain WebUI depends on the board HTTP API.
2. Remote Ubuntu builds explicitly force `CONFIG_SLE_TEAM_WIFI_AP_AUTO_START=y`
   to avoid stale SDK `.config` drift.
3. Remote Ubuntu builds also explicitly request LVGL and an 8-line ST7789 LVGL
   draw buffer.
4. WebSerial command execution now shares one continuous reader and waits for
   command-specific response markers such as `[cfg-json]`.

## Verification

Performed in this iteration:

- `npm --prefix webui test`
- `npm --prefix webui run build`
- `git diff --check -- README.md versions/README.md xc/ws63_team_network/README.md versions/v4.4.31/VERSION.md versions/v4.4.31/MANIFEST.md webui/src/api/client.ts webui/tests/ws63-api-contract.test.mjs scripts/ws63_build_v4_ubuntu.sh xc/ws63_team_network/Kconfig xc/ws63_team_network/src/ws63_team_network_app.c`
- Remote Ubuntu firmware compile through the SOP fallback path:
  Python `paramiko` sync/SSH on `owen@192.168.6.5`, SDK
  `/home/owen/workspace/bearpi-pico_h3863`, then
  `python3 build.py ws63-liteos-app -j4`.

Firmware package:

- `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Size: `1427240` bytes (`1.36 MiB`).

Not performed in this iteration:

- Firmware flash/hardware runtime test.
