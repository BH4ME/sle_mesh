# v4.4.53 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/src/ws63_st7789_display.c`
- `xc/ws63_team_network/src/ws63_st7789_display.h`
- `webui/tests/ws63-api-contract.test.mjs`
- `README.md`
- `versions/README.md`
- `versions/v4.4.53/VERSION.md`
- `versions/v4.4.53/MANIFEST.md`

## Key Deltas

1. Removed the old `ws63_st7789_show_alert()` display API and replaced it with `ws63_st7789_show_event()`.
2. Removed the old `LOST M%u` formatting that exposed decimal internal member IDs such as `M241`.
3. Added display event names and colors for `JOIN`, `LEFT`, `TIMEOUT`, `LOST`, and `REJOIN`.
4. Added leader-side display member state tracking so repeated `joined` callbacks are ignored, first join shows `JOIN`, and return-after-offline shows `REJOIN`.
5. Contract tests now lock the v4.4.53 firmware-visible strings and the no-`LOST M%u` display invariant.

## Verification

Completed on 2026-06-04:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn
git diff --check -- xc/ws63_team_network/src/ws63_team_network_app.c xc/ws63_team_network/src/ws63_st7789_display.c xc/ws63_team_network/src/ws63_st7789_display.h webui/tests/ws63-api-contract.test.mjs README.md versions/README.md versions/v4.4.53/VERSION.md versions/v4.4.53/MANIFEST.md
```

Firmware remote Ubuntu build completed:

```sh
UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS=<local secret> UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 scripts/ws63_build_v4_ubuntu.sh unified
```

The local WSL environment did not have `sshpass`, so the release package was built with the documented fallback path: Python `paramiko` opened `owen@192.168.6.5`, uploaded tarballs for `include/`, `src/`, and `xc/ws63_team_network/`, unpacked them into the SDK, applied the LVGL compatibility patch if needed, configured Kconfig, and ran `python3 build.py -c ws63-liteos-app -j4` on the Ubuntu host.

Remote build result:

- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Output copied to: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1592296` bytes
- Package guard passed for `v4.4.53`, `[display] st7789 ready`, `[team] boot unconfigured`, and `[cfg-json]`
- Stale package guard passed: no `v4.4.37` or `v4.4.48` version strings found

Local verification result:

- `npm --prefix webui test`: passed, `54/54`
- `python -m unittest automation.ws63.tests.test_ws63_auto_burn`: passed, `10/10`
- `git diff --check -- ...`: passed; only Git LF-to-CRLF working-copy warnings were printed

Live COM flashing remains a separate hardware validation step unless completed and recorded in this manifest.
