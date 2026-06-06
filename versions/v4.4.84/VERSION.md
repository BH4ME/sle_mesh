# Version v4.4.84

Date: 2026-06-05

## Scope

`v4.4.84` is a code correction made after re-reading the `v4.4.74` live feedback instead of continuing to flash boards.

Authoritative feedback:

```text
logs/live/v4.4.74_four_board_com16_leader_20260605_135942/leader_COM16.log
APP|Oops:NMI
task:TeamNetworkTask
mepc:0x2669a0
ra:0x266aa6
```

The address mapping recorded in `versions/v4.4.75/VERSION.md` points through the LVGL/ST7789 software-SPI flush path:

```text
hal_gpio_v150_set_output
uapi_gpio_set_val
st7789_mosi
st7789_push_rect
call_flush_cb
draw_buf_flush
```

## Root Cause

`v4.4.83` correctly moved LVGL/ST7789 flushing out of `TeamNetworkTask`, but the new `TeamDisplayTask` was only given a `0x1000` stack.

The original failure call chain had already shown that LVGL plus ST7789 software-SPI GPIO bit-banging is not a tiny background operation. Moving that chain into a smaller stack risked turning the old `TeamNetworkTask` NMI into a new display-task stability issue.

## Fix

- Firmware visible version is bumped to `v4.4.84`.
- `TeamDisplayTask` stack is increased from `0x1000` to `0x1800`, matching the networking task stack size.
- Display work remains isolated from `TeamNetworkTask`.
- Remote build, local WSL build, flash scripts, and auto-burn version guards now expect `v4.4.84`.
- Unit tests now parse the C macros and require `TeamDisplayTask` stack to be at least the `TeamNetworkTask` stack, with display task priority lower than the network task.

## Flash Policy

No board flashing is part of this correction. Flash only after an explicit hardware validation request.

## Required Verification

Run local checks before build or flash:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'<repo-root>'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```
