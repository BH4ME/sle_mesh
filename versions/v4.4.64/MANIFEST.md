# v4.4.64 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.64/VERSION.md`
- `versions/v4.4.64/MANIFEST.md`
- `versions/v4.4.64/FLASH_AND_RELAY_TEST.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `scripts/ws63_flash_multi.ps1`

## Build Guards

Remote Ubuntu build must link these firmware-visible strings:

```text
v4.4.64
cfg direct
runtimeDirectCap
relay failover begin
relay failover holding relay target
relay config notify pending
liveness preserved
```

Expected package:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.64` before flashing.

## Verification Status

Required:

- `git diff --check`
- `.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py`
- `.tooling\py311\python.exe -m unittest discover -s automation\ws63\tests -t .`
- Remote Ubuntu build through `automation/ws63/tools/ws63_remote_build_v4.py`
- Four-port flash with visible per-COM logs.
- Four-board serial join, child reboot, relay reboot, and recovery logs.
