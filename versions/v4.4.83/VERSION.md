# Version v4.4.83

Date: 2026-06-05

## Scope

`v4.4.83` responds to the earlier `v4.4.74` live feedback instead of doing another blind flash.

The `v4.4.74` evidence showed the COM16 leader could reboot with:

```text
APP|Oops:NMI in task TeamNetworkTask
```

Address mapping recorded in `v4.4.75` pointed through LVGL/ST7789 software-SPI flushing:

```text
hal_gpio_v150_set_output
uapi_gpio_set_val
st7789_mosi
st7789_push_rect
call_flush_cb
draw_buf_flush
```

## Root Cause

The previous mitigation only throttled display work while it still ran inside `TeamNetworkTask`.
That reduced pressure but did not remove the risky call chain:

```text
TeamNetworkTask -> ws63_st7789_tick/display flush -> LVGL -> ST7789 soft SPI GPIO bit-bang
```

During serial automation and four-board relay testing, this could still couple screen refresh cost to the group-networking task.

## Fix

- Firmware visible version is bumped to `v4.4.83`.
- ST7789/LVGL tick and pending screen flush now run in a separate low-priority `TeamDisplayTask`.
- `TeamNetworkTask` no longer calls `ws63_st7789_tick()` or `team_display_flush_pending_once()`.
- The networking task only publishes display snapshots and event dirty flags.
- Display status reads `display_status_*` snapshots instead of traversing live member state from the display task.
- Build and burn version guards now default to `v4.4.83`.
- Unit tests now assert that display flushing is isolated from the network task.

## Flash Policy

No flashing is part of this correction. Build/burn only after an explicit hardware validation request.

## Required Verification

Local checks for this version:

```powershell
git diff --check
.\.tooling\py311\python.exe -m py_compile automation\ws63\tools\ws63_four_board_relay_test.py automation\ws63\tools\ws63_remote_build_v4.py automation\ws63\tools\ws63_auto_burn.py automation\ws63\tests\test_ws63_four_board_relay_test.py automation\ws63\tests\test_ws63_auto_burn.py
.\.tooling\py311\python.exe -c "import sys, unittest; sys.path.insert(0, r'<repo-root>'); suite=unittest.defaultTestLoader.loadTestsFromNames(['automation.ws63.tests.test_ws63_four_board_relay_test','automation.ws63.tests.test_ws63_auto_burn']); result=unittest.TextTestRunner(verbosity=2).run(suite); raise SystemExit(0 if result.wasSuccessful() else 1)"
```

## Verification Result

Local checks completed:

```text
git diff --check: PASS
py_compile: PASS
unit tests: PASS, 20 tests
```

The `ws63_auto_burn` unit-test output intentionally logs a stale-package guard
for a temporary fake firmware file. That is expected test evidence and is not a
real flash attempt.
