# Manifest v4.4.83

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `scripts/ws63_flash_multi.ps1`
- `README.md`
- `versions/README.md`
- `meta/PROJECT_OPERATION_SOP.md`
- `versions/v4.4.83/VERSION.md`
- `versions/v4.4.83/MANIFEST.md`

## Notes

This version is a code correction for the `v4.4.74` NMI feedback. It does not include a flash run.

The key invariant is:

```text
TeamNetworkTask does group-networking work.
TeamDisplayTask does LVGL/ST7789 screen work.
```
