# Version v4.4.137

## Type

Firmware robustness release.

## Firmware Version

The WS63 firmware version is now `v4.4.137`.

## Summary

- Advanced firmware and repository records from `v4.4.136` to `v4.4.137`.
- Added member upstream SLE recovery for the case where a configured member keeps failing leader-bound HELLO/packet sends with `reason=NOT_READY`.
- Member upstream `NOT_READY` and upstream `WRITE_FAIL` now trigger a throttled `sle_uart_server_adv_restart()` so the leader or relay can reconnect to the member's SLE server.
- If the member remains without a usable upstream link for `8s`, firmware resets the retry state, forces the next HELLO retry window, and disconnects any stale upstream server connection before restarting advertising again.
- The recovery state is cleared on successful upstream TX, on an active upstream connection, on self join, and during role/leave cleanup.

## Root Cause Addressed

COM15/id51 could be configured as a member while its local upstream SLE server had no active leader/relay connection. In that state the application kept returning `NOT_READY`, so HELLO never reached the leader and the board could remain stuck until a manual reboot.

## Expected Logs

- Short outage:
  - `[sle-tx-fail] type=PACKET dst=<leader> ret=-4 reason=NOT_READY`
  - `[team] member upstream recover reason=not-ready ... adv=0x0 ...`
- Stuck outage:
  - `[team] member upstream recover ... stuck=1 ...`
  - followed by a forced HELLO retry after advertising restarts.
- Recovery:
  - `[team] member upstream ready reason=conn ...`
  - or `[team] member upstream ready reason=joined ...`

## Validation

- Source-level tests cover the firmware version bump and member upstream recovery hooks.
- Remote Ubuntu build guards require the final firmware image to contain `v4.4.137`.
- Remote build completed on `192.168.6.5` using `/home/owen/workspace/bearpi-pico_h3863`.
- Firmware package:
  - `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all_v4.4.137.fwpkg`
  - size: `1614632` bytes
  - SHA256: `CA764CA4274249035A02F6DB82410F62987D4AE9C46DBED9FCE8D6C42067A663`
