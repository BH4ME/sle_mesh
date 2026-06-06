# Version v4.4.6

Date: 2026-05-31

## Positioning

`v4.4.6` is the member-reboot recovery patch on top of `v4.4.5`. It targets the field symptom where turning a member back on after it was lost makes the leader repeatedly connect and disconnect, leaving the leader display at `ON 0 LOST 1`.

## Root Cause

Real-board serial logs showed the leader could see the member advertisement and open a SLE connection, but the member-side server restarted advertising immediately after the upstream connection callback. That produced a repeated `Connected -> adv_restart -> Disconnected` loop before HELLO/ACK could settle, so the member stayed `joined=0` and the leader never returned to `ON 1`.

## What Changed

- Keep member/server advertising stable after a successful upstream connection.
- Restart member/server advertising only after a disconnect, so lost nodes remain discoverable without disrupting a live link.
- Added a contract test that forbids `sle_uart_server_adv_restart()` in the connected branch and requires it in the disconnected branch.
- Move firmware-visible, screen-visible, README, build-script and contract-test version strings to `v4.4.6`.

## Expected Field Behavior

- If a paired member is powered off, the leader should show `ON 0 LOST 1` after the heartbeat timeout.
- When that same member powers back on, the leader should reconnect and return to `ON 1`.
- `LOST 1` remains an accumulated lost-event counter in this version; it does not automatically reset when the member comes back.
