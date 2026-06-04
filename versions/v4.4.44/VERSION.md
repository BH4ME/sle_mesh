# Version v4.4.44

Date: 2026-06-03

## Scope

`v4.4.44` keeps the `v4.4.42` link-loss/manual-leave state-machine fix and adds a burn-time guard against flashing stale firmware packages.

## Root Cause

COM13 and COM16 are visible through the .NET/registry serial enumeration, but the only local unified firmware package currently contains `v4.4.37`. Flashing that package would make live testing invalid because the board would still run old logic while the repository is at `v4.4.44`.

## What Changed

1. `scripts/ws63_flash_team.sh` now checks that the `.fwpkg` contains the expected visible firmware version before flashing.
2. The default expected version is `v4.4.44`.
3. Operators can intentionally override the guard with `EXPECTED_FW_VERSION=` when flashing an old image for recovery.
4. Firmware/build/WebUI contract visible version strings were bumped to `v4.4.44`.

## Expected Behavior

- The current local `v4.4.37` package is refused by default and cannot be accidentally used for `v4.4.44` lifecycle validation.
- A newly built `v4.4.44` package will pass the guard and can be flashed to COM13/COM16 for the live lifecycle test.
