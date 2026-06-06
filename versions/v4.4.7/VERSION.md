# Version v4.4.7

Date: 2026-05-31

## Positioning

`v4.4.7` is the leader-side reconnect stability patch on top of `v4.4.6`.
It targets the field symptom where a rebooted member is detected by the leader,
but the leader repeats `Connected -> Disconnected` and never returns to
`ON 1`.

## Root Cause

Real-board serial logs after flashing `v4.4.6` showed the member/server no
longer restarted advertising on connect, but the leader still disconnected
immediately after `exchange_info` and `discovery ready`.

The remaining difference was on the leader/client path:
`sle_uart_client_handle_connect_state_changed()` restarted SLE seek/scan from
the `SLE_ACB_STATE_CONNECTED` branch. On this WS63 stack that keeps discovery
active while an upstream link is settling, producing a repeated disconnect with
`disc_reason:0x11` before HELLO/heartbeat can stabilize.

## What Changed

- Keep leader/client seek stopped after a successful SLE connection.
- Keep scanning on initial SLE enable, explicit force-rescan, and disconnect
  recovery.
- Added a contract test that forbids `sle_uart_start_scan()` in the leader
  connected branch and requires it in the disconnected branch.
- Move firmware-visible, display-visible, README, build-script and contract-test
  version strings to `v4.4.7`.

## Expected Field Behavior

- If a paired member is powered off, the leader can still show `ON 0 LOST 1`
  after the heartbeat timeout.
- When that same member powers back on or reboots, the leader should reconnect
  without the immediate `Connected -> Disconnected` loop and return to `ON 1`.
- `LOST 1` remains an accumulated lost-event counter. It is not the current
  offline count and may remain visible after the member is online again.
