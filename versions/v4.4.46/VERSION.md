# Version v4.4.46

Date: 2026-06-03

## Scope

`v4.4.46` keeps the member reboot/manual-leave lifecycle logic from `v4.4.45` and fixes the automated system-test burn path so validation builds the v4 unified firmware instead of the older team firmware script.

## Root Cause

The lifecycle code now depends on the v4 unified runtime-role firmware, but `automation/ws63/scripts/ws63_test_system.sh --with-burn` still invoked `scripts/ws63_build_team_ubuntu.sh`. That made an end-to-end "build + burn + test" run capable of producing or selecting the wrong image, which would make COM13/COM16 evidence untrustworthy.

## What Changed

1. The system-test burn stage now calls `scripts/ws63_build_v4_ubuntu.sh unified`.
2. Added a regression test that locks the burn stage to the v4 build script.
3. Moved firmware-visible version strings, build guards, flash guards, direct-burn guards, and WebUI contract checks to `v4.4.46`.

## Expected Behavior

- `ws63_test_system.sh --with-burn` builds the same v4 unified firmware that the project flash script expects.
- Stale `v4.4.37` packages are still refused by both shell and direct Python burn entry points.
- A newly built `v4.4.46` package is required before live COM13/COM16 lifecycle validation can be trusted.
