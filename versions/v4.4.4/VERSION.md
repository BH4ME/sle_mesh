# Version v4.4.4

Date: 2026-05-31

## Positioning

`v4.4.4` is the leader-reboot recovery patch on top of `v4.4.3`. It targets the field symptom where the leader screen stays `ON 0` after only the leader is rebooted, even though the member was already paired before.

## Root Cause

- `v4.4.3` kept approved members only in RAM. After the leader rebooted, the leader came back with `allow=only allow_count=0`, so the member's periodic `HELLO` could not be accepted as an already-approved node.
- During the same restart path, the SLE client could connect physically but remain unusable if service/property discovery reported a transient failure, so business-layer CONFIG/ACK could not be sent back reliably.

## What Changed

- Added NV persistence for the leader allowlist under key `0x5002`.
- Saved the allowlist after CLI/Web pairing approval, pairing stop, and allowlist changes.
- Applied the saved allowlist when a leader role is restored from NV or configured at runtime.
- Marked the SLE UART client ready after successful MTU/profile exchange using the fixed WS63 UART property handle `2`, while still accepting later property-discovery results when available.
- Moved firmware-visible strings and ST7789 title strings to `v4.4.4`.

## Expected Field Behavior

After both boards run `v4.4.4` and the member has been approved once:

- Rebooting only the leader should not require pairing again.
- The leader should reload `allow_count=1` from NV.
- The member should resend `HELLO` automatically and receive CONFIG/ACK.
- The leader display should return to `ON 1`.

## Known Limits

- A board still running `v4.4.3` does not contain this recovery patch.
- If the member was never approved and saved on `v4.4.4`, leader reboot cannot infer approval from history.
- GPS remains pinmap/logging only, not full NMEA parsing.
