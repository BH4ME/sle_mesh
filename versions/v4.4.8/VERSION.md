# Version v4.4.8

Date: 2026-05-31

## Positioning

`v4.4.8` is a focused reconnect debug build after the real-board `v4.4.7`
member reboot test still failed.

## Evidence From v4.4.7

`v4.4.7` correctly kept leader/client seek stopped after connect, and logs
proved the new line was running:

```text
[sle uart client] keep seek stopped after connect
```

The member reboot test still reproduced the failure:

```text
leader: Connected -> exchange_info status:0 -> discovery ready -> pair complete -> Disconnected disc_reason:0x11
member: Connected -> ssaps_mtu_changed -> Disconnected disc_reason:0x10
member: sle-tx-fail ... NOT_READY
```

So `v4.4.7` is not the final recovery fix.

## Root-Cause Hypothesis

The remaining suspicious transition is duplicate SSAP exchange-info on the
leader/client connection:

- The connected callback sends `ssapc_exchange_info_req()`.
- The pair-complete callback also sent another exchange-info request for the
  same `conn_id`.
- The disconnect happens immediately after pair-complete in the real-board log.

`v4.4.8` tests this single variable by allowing only one exchange-info request
per active client connection.

## What Changed

- Add `exchange_requested` state to each SLE UART client connection.
- Route both connect and pair-complete exchange attempts through
  `sle_uart_client_exchange_once()`.
- Log `exchange info already requested` when pair-complete tries to repeat the
  request for a connection that already exchanged MTU.
- Keep the `v4.4.7` seek/advertising stability changes unchanged.
- Move firmware-visible, display-visible, README, build-script and contract-test
  version strings to `v4.4.8`.

## Expected Field Behavior

- Leader may still show `LOST 1` because it is an accumulated lost-event count.
- The important pass condition is that after member reboot, leader returns to
  `ON 1` / `member online=1`, and member returns to `joined=1`.
- If `exchange info already requested ... reason:pair-complete` appears without
  a following 0x11/0x10 disconnect loop, this hypothesis is confirmed.

## Real-Board Result

Failed on the two-board member reboot test.

Baseline after burning and pairing was good:

```text
leader members: member=241 online=1
member state: state=3 joined=1
```

After `cfg reboot` on the member:

```text
leader: exchange info request reason:connect -> pair complete -> disconnect 0x11
member: connected pair_state:0x1 -> mtu_changed mtu_size:208 -> disconnect 0x10
member state: state=2 joined=0
```

The expected `exchange info already requested ... reason:pair-complete` did not
appear. That means duplicate exchange-info was not the trigger. `v4.4.9` moves
the next hypothesis to exchange-info timing: defer it until pairing is complete.
