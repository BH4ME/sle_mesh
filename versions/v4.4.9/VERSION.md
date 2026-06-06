# Version v4.4.9

Date: 2026-05-31

## Positioning

`v4.4.9` is the next focused reconnect fix after the real-board `v4.4.8`
member reboot test still failed.

## Evidence From v4.4.8

The new package was verified and burned to both boards:

```text
package size: 1508520 bytes
contains: v4.4.8, SLE V4.4.8, v4.4.8 board map, exchange info already requested
does not contain: v4.4.7
```

Baseline pairing worked:

```text
leader members: member=241 online=1
member state: state=3 joined=1
```

After rebooting the member, the failure still reproduced:

```text
leader: exchange info request reason:connect -> exchange_info status:0 -> pair complete -> disconnect 0x11
member: connected pair_state:0x1 -> mtu_changed mtu_size:208 -> disconnect 0x10
member state: joined=0
```

`0x11` is local disconnect on the leader and `0x10` is remote disconnect on the
member, so the leader side is closing the link.

The expected `exchange info already requested ... reason:pair-complete` did not
appear, which means the duplicate-exchange hypothesis was not confirmed.

## Root-Cause Hypothesis

The stronger hypothesis is that client exchange-info is sent too early. The
SDK SLE UART sample sends `ssapc_exchange_info_req()` only after
`pair_complete status == 0`, but our previous code sent exchange-info directly
from the `CONNECTED` branch even when `pair_state == SLE_PAIR_NONE`.

## What Changed

- In the leader/client connected branch:
  - if `pair_state == SLE_PAIR_NONE`, request pairing and log the return code;
  - if `pair_state == SLE_PAIR_PAIRED`, exchange immediately as an already
    paired reconnect;
  - otherwise defer exchange-info until pair-complete.
- Log pair-complete `status` on the client path.
- Log `ssaps_set_info()` return code on the server path.
- Keep `v4.4.8` per-connection exchange dedupe.

## Expected Field Behavior

- During member reboot, leader should log `exchange info deferred ... pair_state:0x1`
  before pair-complete.
- If pairing succeeds, leader should then log
  `exchange info request ... reason:pair-complete`.
- Pass condition remains: leader `members` returns `member=241 online=1`, and
  member `state` returns `joined=1`.
