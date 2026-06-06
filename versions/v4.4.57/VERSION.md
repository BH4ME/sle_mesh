# Version v4.4.57

Date: 2026-06-04

## Scope

`v4.4.57` adds hardware-verifiable display event audit logs on top of `v4.4.56`.
It does not change SLE networking behavior; it makes every ST7789/LVGL event refresh visible on the serial log so live tests can prove the screen uses understandable member labels such as `ME7F1` instead of route IDs such as `241`.

## What Changed

1. Added `team_display_event_name()` for stable event names: `JOIN`, `LEFT`, `TIMEOUT`, `LOST`, `REJOIN`.
2. Added `[display-event] event=<name> label=<Mxxxx> member=<route_id> ret=<ret>` after each screen event refresh.
3. Captured event type, member id, label, GPS, and timestamp under the same IRQ lock before flushing, so the audit log matches the displayed event.
4. Updated firmware-visible version strings, build guards, flash guards, and auto-burn default expected version to `v4.4.57`.

## Expected Behavior

- Leader screen events still show `JOIN/LEFT/TIMEOUT/LOST/REJOIN`.
- Member label should be MAC suffix based, for example `ME7F1`.
- Serial logs should include lines like:

```text
[display-event] event=JOIN label=ME7F1 member=241 ret=0 last_seen=...
```

That line means the screen event is for route id `241`, but the user-facing label is `ME7F1`.
