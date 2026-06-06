# Version v4.4.59

Date: 2026-06-04

## Scope

`v4.4.59` fixes the live four-board enrollment blocker found after `v4.4.58`
was flashed and configured on all boards.

`v4.4.58` proved that flash, NV role config, and SLE scan filtering worked:
`COM16` saw `COM13/E7F1` and repeatedly printed `will connect`. The actual
failure was below pairing approval: the SLE client waited for a seek-disable
callback before calling `sle_connect_remote_device()`, but the callback did not
arrive on the live boards. The leader stayed in `seek active conn:0`, and all
members remained `joined=0 / NOT_READY`.

## What Changed

1. Added `sle_uart_client_tick()` to the SLE client and calls it from the common
   team network loop.
2. Added explicit logs for `stop seek for connect ret` and `connect request addr`
   so the serial log proves whether the connect request is issued.
3. Added an 800 ms seek-stop timeout fallback. If `seek_disable_cb` does not
   arrive, the client clears the local seek-active state and requests the
   connection anyway.
4. Kept the normal callback path intact: if `seek_disable_cb` does arrive, it
   still performs the connect immediately.
5. Updated build guards to require both `v4.4.59` and the new connect-recovery
   log strings in the linked ELF.

## Expected Behavior

- `COM16` can be configured as leader.
- `COM13`, `COM17`, and `COM18` can be configured as members from saved NV config.
- Leader scan should no longer get stuck forever at `will connect` plus
  `seek active conn:0`.
- The serial log should show one of:
  - `connect request ... reason:seek-disable-cbk ret:0x0`
  - `seek stop timeout, fallback connect pending ...`
  - `connect request ... reason:seek-stop-timeout ret:0x0`
- After the connect path works, `COM13` can be approved as relay and downstream
  members can be approved through it.

## Required Live Verification

The version is not complete until these are captured in logs:

1. Flash `v4.4.59` to `COM16`, `COM13`, `COM17`, and `COM18`.
2. Confirm each `cfg status` returns `fw=v4.4.59`.
3. Configure `COM16` as leader and the other boards as members.
4. Approve `COM13` route id `241` as relay.
5. Approve `COM17` route id `224` and `COM18` route id `86` as normal members.
6. Confirm leader `members` has all three online and each member `state` has
   `joined=1`.
7. Reboot one downstream member and prove leader sees offline then online again.
8. Reboot relay and prove leader sees relay offline, children self-heal, and the
   recovered original relay behavior is recorded.
