# Version v4.4.90

Date: 2026-06-05

## Scope

`v4.4.90` is a feedback-driven code fix on top of `v4.4.89`. It specifically addresses the `v4.4.74` feedback loop: do not keep flashing when evidence points to stale topology, incomplete cleanup, or user-visible internal route IDs.

## Root Cause

- `cfg clear` and Web factory reset cleared only the saved role config. The saved leader allowlist could survive, so four-board validation could start from a dirty previous mesh.
- The four-board relay test configured roles immediately after firmware/version checks, so a previous relay/child topology could pollute enrollment and failover evidence.
- Serial/Web node rows still exposed internal route IDs as the only stable identifier in some paths, while operators need MAC-suffix labels such as `ME7F1`.
- WebSerial member parsing still expected an older `members` row shape and could miss current relay/tier/max_down rows.

## Fix

- Firmware version bumped to `v4.4.90`.
- `cfg clear` and Web factory reset now clear both saved role config and saved leader allowlist.
- Four-board validation now defaults to clean-start: clear saved role/allowlist state, reboot, verify unconfigured runtime, then configure leader/member roles.
- Serial `members` and `pairing pending` output now includes operator labels derived from MAC suffixes when available.
- WebSerial parsing now accepts current member rows with `label`, `mac/ready`, `relay`, `tier`, and `max_down`.

## Verification

Code-level verification and remote firmware build completed. No firmware burn was run for this change.

- Python unit tests: pass, 26 tests.
- WebUI contract tests: pass, 54 tests.
- `python -m py_compile`: pass.
- `git diff --check`: pass, CRLF normalization warnings only.
- Remote Ubuntu firmware build: pass.
- Built package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`
- Package size: `1603176`
- Package guard: `contains_v4.4.90=True`
- Hardware flash: not run.

## Hardware Runbook

- Flash and four-board validation procedure: `versions/v4.4.90/FLASH_AND_FOUR_BOARD_TEST.md`
