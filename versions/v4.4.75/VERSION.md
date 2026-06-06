# Version v4.4.75

Date: 2026-06-05

## Scope

`v4.4.75` fixes the failure reported after the `v4.4.74` four-board relay test.

The test had already flashed `v4.4.74` and reached the relay recovery phase, but
the COM16 leader rebooted during the run. The authoritative failure evidence is:

```text
log: E:\codex_documents\sle\logs\live\v4.4.74_four_board_com16_leader_20260605_135942\leader_COM16.log
failure: APP|Oops:NMI in task TeamNetworkTask
mepc: 0x2669a0
ra:   0x266aa6
```

Address mapping on the Ubuntu build host showed the NMI occurred while LVGL was
flushing the ST7789 through software SPI:

```text
hal_gpio_v150_set_output
uapi_gpio_set_val
st7789_mosi
st7789_push_rect
call_flush_cb
draw_buf_flush
```

## Root Cause

`TeamNetworkTask` could spin without a fixed yield when serial automation kept
the CLI queue busy. During four-board relay failover tests, that allowed frequent
LVGL timer handling and software-SPI screen flushes inside the network task.

The observed effect was high `TeamNetworkTask` CPU usage followed by NMI while
bit-banging GPIO for ST7789 MOSI. After reboot, the leader lost runtime topology
state, so the test ended with only the recovered relay visible.

## Fix

- Firmware version is bumped to `v4.4.75`.
- `TeamNetworkTask` now sleeps `SLE_TEAM_MAIN_LOOP_SLEEP_MS` every loop so it
  always yields CPU even when CLI commands are frequent.
- CLI queue timeout is shortened to 20 ms so the added main-loop sleep does not
  make command processing sluggish.
- LVGL timer handling is limited by `ST7789_LVGL_HANDLER_MIN_INTERVAL_MS`.
- ST7789 status refresh is limited to 500 ms to avoid excessive software-SPI
  redraw during repeated `members` polling.
- Build and flash guards now default to `v4.4.75`.

## Expected Four-Board Policy

Target topology is unchanged:

```text
COM16: leader
COM13: member/relay candidate
COM17: member child
COM18: member child
leader direct cap: 1
```

Relay recovery policy remains target based:

```text
direct capacity controls the number of children that must use relay
relay target is recalculated from online count
when original relay returns, the leader keeps the required relay count
if only one relay is needed, the best relay remains relay and the other becomes member
```

## Required Verification

Before treating this version as final, run:

```powershell
.\.tooling\py311\python.exe -m unittest `
  automation.ws63.tests.test_ws63_four_board_relay_test `
  automation.ws63.tests.test_ws63_auto_burn
```

Then build on `192.168.6.5`, flash all four ports with software reset, and rerun
the four-board relay test.
