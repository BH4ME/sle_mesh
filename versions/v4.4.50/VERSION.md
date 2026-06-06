# Version v4.4.50

Date: 2026-06-04

## Scope

`v4.4.50` is a repository/test-gate version for live relay validation. It does not change the firmware-visible version string, board pinmap, or burned firmware image. The live-tested firmware on COM13/COM16 remains `v4.4.48`.

This version adds a reusable three-board relay-cycle test so the relay signal-loss requirement can be proven on hardware when a leader, relay member, and child member are available.

## What Changed

1. Added `automation/ws63/tools/ws63_relay_cycle_test.py` for live leader + relay + child failover testing.
2. Added unit tests that prove the relay-cycle tool reboots the relay instead of sending `leave`, and does not manually reconfigure the child after relay loss.
3. Added `--with-relay-cycle` to `automation/ws63/scripts/ws63_test_system.sh`.
4. Updated version records so the repository record is `v4.4.50` while firmware-visible version remains `v4.4.48`.

## Expected Behavior

- Requirements 1 and 2 remain covered by the two-board link-cycle hardware test.
- Requirement 3 is covered by the existing 30-node simulator gate and can now be validated live with:

```sh
bash automation/ws63/scripts/ws63_test_system.sh --with-relay-cycle --ports <leader>,<relay>,<child>
```

- By default, the live relay-cycle test requires the child to initially select the relay as its upstream parent. If the boards are physically too close and the child directly selects the leader, the test fails because the field condition is not proven.
