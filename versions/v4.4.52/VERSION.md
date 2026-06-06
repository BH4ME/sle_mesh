# Version v4.4.52

Date: 2026-06-04

## Scope

`v4.4.52` is a repository/test-gate version. It does not change the firmware-visible version string, board pinmap, or burned firmware image. The current firmware-visible version remains `v4.4.48`.

This version adds a real serial-port hardware preflight for the live three-board relay-cycle test.

## What Changed

1. Added `automation/ws63/tools/ws63_serial_preflight.py`.
2. The preflight lists actual `pyserial` ports, filters out system ports such as `COM1`, and checks that requested relay-cycle ports look like board UARTs.
3. `automation/ws63/scripts/ws63_test_system.sh --with-relay-cycle` now runs the serial preflight before starting the hardware test.
4. Added unit-test coverage for the serial preflight tool and system-script integration.

## Expected Behavior

- With only `COM13` and `COM16` board ports available, relay-cycle preflight fails clearly instead of running a partial three-board test.
- With three board ports available, preflight prints the exact relay-cycle command to run.
- The live relay-cycle requirement still requires a third board connected as leader + relay + child.
