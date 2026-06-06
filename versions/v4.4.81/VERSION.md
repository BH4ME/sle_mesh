# Version v4.4.81

Date: 2026-06-05

## Scope

`v4.4.81` keeps the `v4.4.80` direct next-hop firmware fix and strengthens the
four-board live test so the next hardware run automatically fails on the
`v4.4.74`-style `NO_ROUTE` regression.

This version is a verification-flow iteration, not a new topology policy.

## Root Cause Guarded

The earlier failure pattern was:

```text
leader: route hint member=<id> parent=<relay> ret=-4
relay:  [sle-rx] HELLO <member>-><leader>
relay:  [sle-tx-fail] type=PACKET dst=<leader> ret=-4 reason=NO_ROUTE
```

If either pattern appears during the COM16 leader four-board test, the test now
records route regression evidence and fails instead of leaving the issue buried
in saved serial logs.

## Fix

- Firmware visible version is bumped to `v4.4.81`.
- Four-board test default expected firmware is `v4.4.81`.
- Added route-regression log scanning to `ws63_four_board_relay_test.py`.
- Added unit tests for route-hint `NO_ROUTE` and relay leader-bound `NO_ROUTE`
  detection.

## Runtime Status

This version has not been flashed in this turn. The next required step for the
full objective remains hardware validation on COM16/COM13/COM17/COM18 after the
user explicitly asks to burn/test.
