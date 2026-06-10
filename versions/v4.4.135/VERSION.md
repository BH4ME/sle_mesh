# Version v4.4.135

## Type

Firmware relay leave/rejoin recovery release.

## Firmware Version

The WS63 firmware version is now `v4.4.135`.

## Summary

- Advanced firmware and current repository records from `v4.4.134` to `v4.4.135`.
- Preserved the v4.4.134 hardware profile, display, battery, charger, and RGB blink behavior.
- Added relay-child cleanup on successful member `leave`, so a relay that leaves the team disconnects downstream children before its runtime state is cleared.
- Updated build, flash, and automation defaults to expect `v4.4.135`.

## Root Cause

- The previous natural relay leave/rejoin test showed that when the active relay left and later rejoined as a non-relay member, one downstream member could remain physically connected to the old parent.
- Existing firmware already dropped downstream children when a leader CONFIG demoted a relay, but the local member `leave` path reset state without first releasing downstream relay-child connections.
- The stale physical child connection could keep the child sending HELLO traffic through a non-relay parent, where leader-bound unicast packets were rejected instead of causing an immediate parent reselect.

## Fix

- `team_member_reset_after_leave()` now calls `team_member_drop_relay_children("member-leave")` before clearing runtime role state.
- Existing `config-demote` behavior remains in place for leader-driven relay revocation.
- Build guards and source-level tests now require the `member-leave` cleanup path.

## Validation

- Source-level tests assert that `team_member_reset_after_leave()` drops relay children before clearing `role_configured`.
- Build guards require the final firmware image to contain `v4.4.135` and the `member-leave` cleanup string.
- Live validation target: flash all current responsive boards with `v4.4.135` and rerun the natural relay leave/rejoin recovery test without manual relay intervention.
