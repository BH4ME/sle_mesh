# v4.4.56 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.56/VERSION.md`
- `versions/v4.4.56/MANIFEST.md`
- `webui/tests/ws63-api-contract.test.mjs`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

## Key Deltas

1. Added `display_member_last_events[]` and `team_display_member_last_event()`.
2. Allowed `conn_disconnected` to emit a `LOST` display event even when the protocol record is already offline.
3. Suppressed duplicate disconnect display after manual `LEFT`.
4. Prevented duplicate relay-offline callback when the disconnect path is only replaying an already-offline display event.
5. Added ELF guards for `v4.4.56` and `already_offline=%u`.

## Verification

Completed on 2026-06-04:

```sh
npm --prefix webui test
python -m unittest automation.ws63.tests.test_ws63_auto_burn
git diff --check -- README.md versions/README.md versions/v4.4.56/VERSION.md versions/v4.4.56/MANIFEST.md webui/tests/ws63-api-contract.test.mjs xc/ws63_team_network/src/ws63_team_network_app.c scripts/ws63_build_v4_ubuntu.sh scripts/ws63_build_v4_local_wsl.sh scripts/ws63_flash_team.sh automation/ws63/tools/ws63_auto_burn.py automation/ws63/tests/test_ws63_auto_burn.py
```

Results:

- `npm --prefix webui test`: pass, `55/55`.
- `python -m unittest ...`: pass, `27/27`.
- `git diff --check`: pass with only Windows LF-to-CRLF warnings.

Remote Ubuntu build target used:

```sh
UBUNTU_HOST=192.168.6.5 UBUNTU_USER=owen UBUNTU_PASS=<local secret> UBUNTU_SDK=/home/owen/workspace/bearpi-pico_h3863 BUILD_JOBS=4 scripts/ws63_build_v4_ubuntu.sh unified
```

Remote build result:

- Host: `owen@192.168.6.5`
- SDK: `/home/owen/workspace/bearpi-pico_h3863`
- Package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1594344` bytes
- Post-build guard: pass for `v4.4.56`, `[display] st7789 ready`, `[team] boot unconfigured`, `[cfg-json]`, `[team] disconnect lookup`, `already_offline=%u`, and `reason=conn_disconnected already_offline=1 last_event=LEFT`.
