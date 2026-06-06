# Manifest: v4.4.96

## Repository Organization

- `README.md`
- `docs/README.md`
- `docs/repository_layout.md`
- `docs/version_management.md`
- `firmware/README.md`
- `scripts/README.md`
- `tools/README.md`
- `automation/ws63/README.md`
- `versions/README.md`
- `versions/v4.4.96/VERSION.md`
- `versions/v4.4.96/MANIFEST.md`
- `.gitignore`
- `.gitattributes`

## Script Layout

- `scripts/build/ws63_build_team_ubuntu.sh`
- `scripts/build/ws63_build_team_vm.sh`
- `scripts/build/ws63_build_v4_local_wsl.sh`
- `scripts/build/ws63_build_v4_ubuntu.sh`
- `scripts/flash/ws63_flash_multi.ps1`
- `scripts/flash/ws63_flash_team.sh`
- `scripts/serial/ws63_serial_cfg.ps1`
- `scripts/sim/simulate_20_members.sh`
- `scripts/sim/simulate_python_1v20.sh`
- `scripts/sim/simulate_v2.sh`
- `scripts/test/ws63_test_system.sh`
- `scripts/review/run_review_with_service.sh`

## Hardware Publication

- `hardware/README.md`
- `hardware/boards/README.md`
- `hardware/schematics/README.md`
- `hardware/enclosures/README.md`
- `hardware/enclosures/sle-pcb-enclosure/README.md`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/README.md`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/MANIFEST.md`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/source/sle_enclosure_v1.1.4_source.py`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/step/*.step`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/stl/*.stl`
- `hardware/enclosures/sle-pcb-enclosure/v1.1.4/preview/*.png`

## Updated Tests And Automation References

- `automation/ws63/scripts/ws63_test_system.sh`
- `automation/ws63/tests/test_ws63_system_script.py`
- `automation/ws63/tools/ws63_flash_bind_team.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `webui/tests/ws63-api-contract.test.mjs`

## Removed Legacy Wrappers

- `tools/ws63_auto_burn.py`
- `tools/ws63_flash_bind_team.py`
- `tools/ws63_link_cycle_test.py`
- `tools/test_ws63_auto_burn.py`
- `tools/test_ws63_flash_bind_team.py`
- `tools/test_ws63_link_cycle_test.py`
