# v4.4.54 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.54/VERSION.md`
- `versions/v4.4.54/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `xc/ws63_team_network/lv_conf.h`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

## Key Deltas

1. Added a more readable ST7789/LVGL `LINK-MESH` panel layout for leader/member lifecycle events.
2. Kept the v4.4.53 identity fix: event labels use MAC-suffix route labels such as `Mxxxx`, not decimal internal ids such as `M241`.
3. Added LVGL status/event panels, accent rails, wrapped event text, and event-specific hints/colors.
4. Increased `LV_MEM_SIZE` to `20KB` to support the extra LVGL objects.
5. Updated build, flash, and automation version guards to `v4.4.54`.

## Verification

Completed on 2026-06-04:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn
git diff --check -- README.md versions/README.md versions/v4.4.54/VERSION.md versions/v4.4.54/MANIFEST.md webui/tests/ws63-api-contract.test.mjs xc/ws63_team_network/lv_conf.h xc/ws63_team_network/src/ws63_st7789_display.c xc/ws63_team_network/src/ws63_team_network_app.c scripts/ws63_build_v4_ubuntu.sh scripts/ws63_build_v4_local_wsl.sh scripts/ws63_flash_team.sh automation/ws63/tools/ws63_auto_burn.py automation/ws63/tests/test_ws63_auto_burn.py
```

Remote Ubuntu build target:

```sh
UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS=<local secret> UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 scripts/ws63_build_v4_ubuntu.sh unified
```

Live COM flashing remains a separate hardware validation step unless completed and recorded in this manifest.

Remote Ubuntu build completed with Python `paramiko` fallback because the local Windows host is not relying on `sshpass`/`rsync`:

```sh
Host: owen@192.168.6.5
SDK: /home/owen/workspace/bearpi-pico_h3863
Build: python3 build.py -c ws63-liteos-app -j4
```

Remote build result:

- Output copied to: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1593704` bytes
- Post-build guard passed for `v4.4.54`, `LINK-MESH`, `[display] st7789 ready`, `[team] boot unconfigured`, `[cfg-json]`, `NODE ONLINE`, `HEARTBEAT T/O`, and `BACK ONLINE`
- Stale package guard passed: no `v4.4.37`, `v4.4.48`, `v4.4.53`, or `LOST M%u` strings found in the ELF

Memory summary from the successful remote build:

- `ITCM`: `13256 B / 16 KB`, `80.91%`
- `DTCM`: `14844 B / 16 KB`, `90.60%`
- `SRAM`: `218672 B / 548608 B`, `39.86%`
- `PROGRAM`: `1433092 B / 2357504 B`, `60.79%`

Local verification result:

- `npm --prefix webui test`: passed, `54/54`
- `python -m unittest automation.ws63.tests.test_ws63_auto_burn`: passed, `10/10`
- `git diff --check -- ...`: passed; only Git LF-to-CRLF working-copy warnings were printed

Live COM flashing remains a separate hardware validation step unless completed and recorded in this manifest.
