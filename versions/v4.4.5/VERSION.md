# Version v4.4.5

Date: 2026-05-31

## Positioning

`v4.4.5` is a deployment-blocker fix on top of `v4.4.4`. It keeps the leader reboot recovery design, but fixes the real-board issue where `cfg leader now` and `cfg member now` saved NV successfully and then returned `ret=-4`, so the role was not applied immediately.

## Root Cause

On the boards tested after flashing `v4.4.4`, `uapi_nv_write()` stored the config and a later `cfg status` could read it back, but `uapi_nv_flush()` returned `0x80000002`. The old code overwrote the successful write result with the flush result, so the caller treated the save as failed and skipped runtime apply.

## What Changed

- Treat `uapi_nv_flush()` return as a warning after a successful `uapi_nv_write()`.
- Log both NV write result and flush result as `ret=... flush=...`.
- Apply the same non-fatal flush behavior to leader allowlist persistence.
- Add `-Mode apply` to `scripts/ws63_serial_cfg.ps1` so saved config can be applied from the Windows helper.
- Move firmware-visible and screen-visible strings to `v4.4.5`.

## Expected Field Behavior

- `cfg leader now 1 17` should save NV and start leader runtime immediately.
- `cfg member now 279A 1 17` should save NV and start member runtime immediately.
- `cfg apply` should be available from the helper if a saved config needs to be started later.
- The `v4.4.4` leader-reboot recovery behavior remains the target: after approving a member once, leader reboot should come back to `ON 1`.
