# Version v4.4.49

Date: 2026-06-04

## Scope

`v4.4.49` is a repository/test-gate version. It does not change the firmware-visible version string, board pinmap, or burned firmware image. The live-tested firmware on COM13/COM16 remains `v4.4.48`.

This version makes the relay failover requirement harder to regress by promoting the 30-node relay scenario from a one-off manual simulation into automated test coverage.

## What Changed

1. Added a 30-node Python simulator regression test for relay failover, relay reselection, leaf reparenting, relay recovery, and `lost_parent=0`.
2. Added `sim_python_1v30_relay_failover` to the WS63 system quick test path.
3. Changed `scripts/simulate_v2.sh` Python log naming from fixed `python_1v20.log` to member-count-aware `python_1v<N>.log`.
4. Kept the firmware-visible version at `v4.4.48` because no new firmware image was built or burned for this test-gate-only change.

## Expected Behavior

- `bash automation/ws63/scripts/ws63_test_system.sh --quick` now includes the 30-node relay failover gate.
- Running `scripts/simulate_v2.sh --suite=python --py-members=30 ...` writes `logs/sim/python_1v30.log`.
- A relay signal-loss event causes children to reparent through another relay or the leader without reporting `lost_parent`.
