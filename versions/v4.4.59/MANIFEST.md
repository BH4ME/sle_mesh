# v4.4.59 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.59/VERSION.md`
- `versions/v4.4.59/MANIFEST.md`
- `versions/v4.4.59/FLASH_AND_RELAY_TEST.md`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.c`
- `xc/ws63_team_network/sle_uart_client/sle_uart_client.h`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_flash_multi.ps1`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`

`v4.4.58` files remain as a historical rollback record. Do not delete or rewrite
that version directory when publishing `v4.4.59`.

## Build Guards

Remote Ubuntu build must link these firmware-visible strings:

```text
v4.4.59
seek stop timeout, fallback connect pending
connect request addr:
```

Expected package:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.59` before flashing.

## Verification Status

Pending:

- Local unit/simulation tests.
- Remote Ubuntu build.
- Four-port flash.
- Four-board join and relay failover live test.
