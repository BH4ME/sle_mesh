# v4.4.60 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.60/VERSION.md`
- `versions/v4.4.60/MANIFEST.md`
- `versions/v4.4.60/FLASH_AND_RELAY_TEST.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `webui/src/protocol/types.ts`
- `webui/src/api/client.ts`
- `webui/tests/ws63-api-contract.test.mjs`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_flash_multi.ps1`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

## Build Guards

Remote Ubuntu and local WSL builds must link these firmware-visible strings:

```text
v4.4.60
seek stop timeout, fallback connect pending
connect request addr:
cfg direct
runtimeDirectCap
plan=%u
```

Expected package:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.60` before flashing.

## Verification Status

Required:

- `git diff --check`
- `.tooling\py311\python.exe -m unittest automation.ws63.tests.test_ws63_auto_burn`
- `npm --prefix webui test`
- Remote Ubuntu build through `scripts/ws63_build_v4_ubuntu.sh unified`
- Four-port flash with live per-COM logs.
- Four-board serial join, child reboot, relay reboot, and recovery logs.
