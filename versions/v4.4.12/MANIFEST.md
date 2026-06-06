# v4.4.12 Manifest

## Changed Files (this iteration)

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `README.md`
- `versions/README.md`
- `xc/ws63_team_network/README.md`
- `versions/v4.4.12/VERSION.md`
- `versions/v4.4.12/MANIFEST.md`

## Key Logic Deltas

1. Runtime entry refactor (behavior-preserving):
   - Split task bootstrap from loop body.
   - Split loop common tick from role-configured tick.
   - Split entry prestart from task spawn.

2. Readability/maintainability improvements:
   - `team_network_task` now only orchestrates phases.
   - Leader/member runtime branching moved to dedicated helper.

3. Version bump:
   - firmware/hardware tag updated to `v4.4.12`.

## Verification

Performed:

- Source diff review for touched areas.
- `git diff --check` (targeted files).

Not performed in this iteration:

- Full WS63 firmware compile/flash.
- On-device runtime validation.
