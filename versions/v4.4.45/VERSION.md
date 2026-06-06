# Version v4.4.45

Date: 2026-06-03

## Scope

`v4.4.45` keeps the link-loss/manual-leave lifecycle logic and extends stale-firmware protection to the direct Python burn tool.

## Root Cause

The shell flashing script can reject stale `.fwpkg` files, but direct calls to `automation/ws63/tools/ws63_auto_burn.py` could still bypass that guard. Since the only local unified package currently contains `v4.4.37`, direct flashing would invalidate live validation for the current code.

## What Changed

1. Added `--expected-version` to `ws63_auto_burn.py`.
2. Default direct-burn expected version is `v4.4.45`, also overridable with `EXPECTED_FW_VERSION`.
3. Version checking happens before loading vendor burn tools so stale packages are rejected even when burn tooling is missing locally.
4. Added unit tests for package version detection, stale package refusal, default expected version, and show-mode behavior.

## Expected Behavior

- Direct Python flashing refuses the current local `v4.4.37` package by default.
- Shell flashing and direct Python flashing now both require the expected visible firmware version unless intentionally overridden.
- A newly built `v4.4.45` package can be flashed to COM13/COM16 for live lifecycle validation.
