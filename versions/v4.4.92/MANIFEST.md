# Manifest v4.4.92

Date: 2026-06-05

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `automation/ws63/tests/test_ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_four_board_relay_test.py`
- `automation/ws63/tools/ws63_auto_burn.py`
- `automation/ws63/tools/ws63_remote_build_v4.py`
- `automation/ws63/tests/test_ws63_auto_burn.py`
- `README.md`
- `versions/README.md`
- `versions/v4.4.92/VERSION.md`
- `versions/v4.4.92/MANIFEST.md`
- `versions/v4.4.92/FLASH_AND_FOUR_BOARD_TEST.md`
- `meta/PROJECT_OPERATION_SOP.md`

## Notes

This is a code correction for the v4.4.91 four-board evidence:

```text
[sle-rx] HELLO 224->154 ...
[team] node packet role=0 len=26 ret=-4
```

The fix keeps relay demand tied to known deployment size, not only currently visible online/pending nodes.

## Verification

Pending until local tests and remote build are run.
