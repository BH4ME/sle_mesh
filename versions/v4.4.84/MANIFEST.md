# Manifest v4.4.84

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_multi.ps1`
- `scripts/ws63_flash_team.sh`
- `scripts/ws63_build_v4_ubuntu.sh`
- `scripts/ws63_build_v4_local_wsl.sh`
- `README.md`
- `versions/README.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/v4.4.84/VERSION.md`
- `versions/v4.4.84/MANIFEST.md`

## Notes

This version closes the `v4.4.74` NMI feedback loop at the code level.

Key invariants:

```text
TeamNetworkTask must not call LVGL/ST7789 flush/tick functions.
TeamDisplayTask owns LVGL/ST7789 flush/tick functions.
TeamDisplayTask stack must be >= TeamNetworkTask stack.
```

No flash run was performed for this version during the code-fix turn.
