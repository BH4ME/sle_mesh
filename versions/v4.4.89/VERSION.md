# Version v4.4.89

Date: 2026-06-05

## Scope

`v4.4.89` is a small feedback-driven fix on top of `v4.4.88`.

The user feedback from the earlier `v4.4.74` hardware run was that the leader/display
must not expose internal decimal route IDs such as `M241` as the user-facing node
identity. Operators need to see the stable board suffix, for example `ME7F1`, so a
lost/rejoin event can be matched to the physical node.

## Root Cause

The firmware already preferred MAC suffix labels when the member or pending-member
record still had `mac_ready=1`. However, display event formatting could still fall
back to an internal route id if the current member lookup did not have a MAC at the
moment the event was rendered.

That made diagnostics confusing during relay loss, reboot, and recovery because the
screen could show an implementation route id instead of the board suffix.

## Fix

- Firmware visible version is bumped to `v4.4.89`.
- ST7789/LVGL display code now caches the last known member label per route id.
- Event rendering uses the cached MAC suffix label before falling back to an internal
  route id.
- The `v4.4.88` route fix is preserved: leader-side relayed `ROUTE_UPDATE` packets keep
  the physical first hop as next-hop.

## Verification

Completed before handoff:

- `git diff --check`: pass, with CRLF normalization warnings only.
- Python `py_compile`: pass.
- Python unit tests: pass, 20 tests.
- Remote Ubuntu firmware build: pass.

Build output:

```text
output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg
package size=1602792
contains_v4.4.89=True
```

No flashing was performed for this version because the current request was to stop
blind flashing, review the earlier feedback, and modify code.
