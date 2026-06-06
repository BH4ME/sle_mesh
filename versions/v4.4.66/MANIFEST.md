# v4.4.66 Manifest

## Changed Files

- `README.md`
- `versions/README.md`
- `versions/v4.4.66/VERSION.md`
- `versions/v4.4.66/MANIFEST.md`
- `versions/v4.4.66/FLASH_AND_RELAY_TEST.md`
- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `scripts/ws63_flash_multi.ps1`

## Firmware Guards

Remote Ubuntu build must link these firmware-visible strings:

```text
v4.4.66
cfg direct
runtimeDirectCap
relay failover begin
relay failover holding relay target
relay config notify pending
liveness preserved
```

Source-level tests must also verify:

```text
team_leader_should_seek_member
pairing_enabled != 0U
team_leader_find_member_slot(candidate_id) != NULL
sle_team_node_is_member_allowed(&g_team_node, candidate_id)
```

## Expected Package

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
```

The package must contain `v4.4.66` before flashing.
