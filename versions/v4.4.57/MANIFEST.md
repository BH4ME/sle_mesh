# v4.4.57 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.57/VERSION.md`
- `versions/v4.4.57/MANIFEST.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

## Verification

Completed on 2026-06-04:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_link_cycle_test automation.ws63.tests.test_ws63_serial_preflight automation.ws63.tests.test_ws63_system_script
python -m tools.test_sle_team_python_sim
git diff --check -- README.md versions/README.md versions/v4.4.57/VERSION.md versions/v4.4.57/MANIFEST.md xc/ws63_team_network/src/ws63_team_network_app.c scripts/ws63_build_v4_ubuntu.sh scripts/ws63_build_v4_local_wsl.sh scripts/ws63_flash_team.sh automation/ws63/tools/ws63_auto_burn.py automation/ws63/tests/test_ws63_auto_burn.py webui/tests/ws63-api-contract.test.mjs
```

Results:

- `npm --prefix webui test`: pass, `55/55`.
- WS63 automation unittest group: pass, `24/24`.
- Protocol simulation tests: pass, `8/8`.
- `git diff --check`: pass with only Windows LF-to-CRLF warnings.

Remote Ubuntu build:

- Host: `owen@192.168.6.5`
- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1594472` bytes
- Package guards found `v4.4.57`, `[display-event] event=%s label=%s member=%u`, `[team] disconnect lookup`, `already_offline=%u`, `[cfg-json]`, and `[display] st7789 ready`.

Live-board verification:

- Burned `COM16` and `COM13` successfully with `v4.4.57`.
- Confirmed both boards report `fw:"v4.4.57"` and `disp ready=1`.
- Configured `COM16` as leader `L279A`, `COM13` as member `ME7F1`.
- Verified `JOIN`, `LEFT`, `REJOIN`, `LOST`, and `TIMEOUT` display audit events all use `label=ME7F1` while retaining `member=241` only as the internal route id.
- Raw logs saved under `logs/display_event_v4_4_57/20260604_145432/`.
