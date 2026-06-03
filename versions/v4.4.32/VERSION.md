# Version v4.4.32

Date: 2026-06-02

## Scope

`v4.4.32` fixes the local C core simulation failure in the leader pairing
window. The failure showed that an unapproved member could bypass pending
approval when pairing was open and the allowlist was empty.

## Root Cause Evidence

1. `sle_team_node_pairing_start()` enables `member_filter_enabled`.
2. `sle_team_node_is_member_allowed()` intentionally treats an empty allowlist
   as allow-all outside pairing, preventing lockout after reboot or startup.
3. During pairing, that same empty-allowlist fallback incorrectly allowed an
   unknown member HELLO to create an online member record immediately instead
   of staging it in `pending_members`.
4. The local C core test reproduced this at
   `examples/team_network_demo.c:264`, where member `2` was expected to remain
   pending but had already joined.

## What Changed

1. `sle_team_handle_hello()` now stages unknown member HELLO packets into
   `pending_members` when pairing is enabled, filtering is enabled, and the
   allowlist is empty.
2. The empty-allowlist allow-all behavior remains intact outside the pairing
   staging case, so approved/known members and reboot recovery are not locked
   out.
3. Firmware-visible version strings and project docs are synchronized to
   `v4.4.32`.

## Verification

Performed locally on this Windows PC using MSYS2 UCRT64 GCC `16.1.0`:

- `scripts/simulate_v2.sh --suite=all --stress=1`
- `scripts/simulate_v2.sh --suite=python --stress=10 --py-members=30 --py-direct-cap=8 --py-relay-target=3 --py-fail-tick=6 --py-recover-tick=10 --py-ticks=18 --py-packet-loss-rate=0.2 --py-jitter-min-ms=10 --py-jitter-max-ms=120 --py-batch-fail-relay-count=1 --py-batch-fail-relay-ticks=6`

Both passed.

## Known Limits

1. This version was verified with local simulations only.
2. Firmware flash and live board runtime verification still require a separate
   on-device run after compile.
