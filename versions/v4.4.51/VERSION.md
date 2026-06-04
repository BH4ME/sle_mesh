# Version v4.4.51

Date: 2026-06-04

## Scope

`v4.4.51` is a repository/test-gate version. It does not change the firmware-visible version string, board pinmap, or burned firmware image. The current firmware-visible version remains `v4.4.48`.

This version tightens the live relay-cycle automation so missing three-board hardware is reported immediately before unit/simulation work starts.

## What Changed

1. Added relay-cycle preflight validation in `automation/ws63/scripts/ws63_test_system.sh`.
2. `--with-relay-cycle` now requires `--ports leader,relay,child` and fails early when fewer than three ports are supplied.
3. Added unit-test coverage for the relay-cycle preflight guard.
4. Kept the firmware-visible version at `v4.4.48` because this is an automation-only change.

## Expected Behavior

- Two-board lifecycle testing still uses `ws63_link_cycle_test.py`.
- Three-board relay testing uses:

```sh
bash automation/ws63/scripts/ws63_test_system.sh --with-relay-cycle --ports <leader>,<relay>,<child>
```

- If only two ports are provided, the script exits early with:

```text
--with-relay-cycle requires three ports: leader,relay,child
```
