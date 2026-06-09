# Manifest: v4.4.100

## Changed Areas

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `xc/ws63_team_network/Kconfig`
- `include/sle_team_node.h`
- `src/sle_team_node.c`
- `scripts/build/ws63_build_v4_local_wsl.sh`
- `scripts/build/ws63_build_v4_ubuntu.sh`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_five_board_member_loss_test.py`
- `scripts/flash/ws63_flash_multi.ps1`
- `scripts/flash/ws63_flash_team.sh`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- Version and README records pointing to `versions/v4.4.100/`.

## Hardware Constants

- GPS modules are not populated on the tested boards; firmware default is GPS disabled.
- Optional GPS UART pinmap, when enabled for future hardware: UART1, TX IO17, RX IO18.
- Battery ADC control: IO5.
- Battery ADC input: IO12 / ADC channel 5.
- Divider: R8 390k, R9 100k.
- Conversion: `VBAT_mV = ADC_mV * 490 / 100`.
- Percent clamp: 3300mV to 4200mV.

## Verification Commands

```powershell
.\.tooling\py311\python.exe -m unittest automation.ws63.tests.test_ws63_auto_burn automation.ws63.tests.test_ws63_four_board_relay_test
```

```powershell
wsl bash -lc 'cd /mnt/e/codex_documents/sle && scripts/build/ws63_build_v4_local_wsl.sh unified'
```

Run the build before flashing production boards; the source tests prove the guards and constants, while the firmware build proves SDK ADC linkage.
