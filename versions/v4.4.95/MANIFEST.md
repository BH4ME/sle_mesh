# v4.4.95 Manifest

## Version Strings

- Firmware version: `v4.4.95`
- Hardware constraints string: `v4.4.95 pairing allowlist preserve`

## Main Files

- `src/sle_team_node.c`
  - Preserves already-online members in `sle_team_node_pairing_stop()` before pending-member approval.
- `xc/ws63_team_network/src/ws63_team_network_app.c`
  - Carries the visible v4.4.95 version and constraint strings.
- `automation/ws63/tools/ws63_four_board_relay_test.py`
  - Supports natural member mode with `--skip-direct-config --natural-members`.
- `automation/ws63/tools/ws63_remote_build_v4.py`
  - Guards the remote build package for v4.4.95 and retained relay behavior strings.
- `scripts/ws63_flash_multi.ps1`
  - Defaults expected flash version to v4.4.95.

## Guard Strings

The firmware package should contain:

- `v4.4.95`
- `v4.4.95 pairing allowlist preserve`
- `runtimeRelayBudget`
- `relay swap observe`
- `swap-promote`
- `swap-demote`

## Hardware Evidence

Completed. Evidence roots:

- Burn: `logs/burn/v4.4.95_<timestamp>`
- Live natural run: `logs/live/v4.4.95_natural_default_direct_<timestamp>`

Specific logs:

- Remote build: `logs/build/v4.4.95_remote_20260606_130024/remote_build.log`
- Burn: `logs/burn/v4.4.95_20260606_130733`
- Live natural run: `logs/live/v4.4.95_natural_default_direct_20260606_131107`

## Package

- Path: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Size: `1,604,520` bytes
- SHA256: `2EF3066BDCD21D7F8125BDD3B714302C980D5ADF8364F9590E2D088C2D6D1F77`

## Natural Run Summary

- COM16 was configured as leader.
- COM13, COM17, and COM18 were configured as normal members.
- The test used `--skip-direct-config --natural-members`, so no `cfg direct` command was sent and no member was manually pre-approved as relay.
- Leader reported `runtimeDirectCap=8` and `runtimeRelayBudget=3`.
- Firmware kept all three members direct: `active=3 direct=3 relayed=0`.
- No relay was elected (`relays=[]`), so relay reboot/failover/recovery was not applicable in this natural-capacity topology.
