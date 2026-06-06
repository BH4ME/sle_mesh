# Version v4.4.91

Date: 2026-06-05

## Scope

`v4.4.91` is a code-only correction after re-reading the earlier `v4.4.74` feedback chain. No flashing is part of this change.

The key lesson from `v4.4.74` was that repeated flashing did not solve code-level faults: NMI risk, route bookkeeping mistakes, and confusing internal node labels had to be fixed in firmware logic first.

## Root Cause

The protocol state machine already copied packet bodies before reading route and alert structures. However, the WS63 board adapter still parsed `ROUTE_UPDATE` bodies in two places by casting `uint8_t *` packet payloads directly to `sle_team_route_update_body_t *`.

The current route-update body is four bytes, so this may not fail immediately. But it is inconsistent with the safer parser style used elsewhere and can become a hidden MCU alignment/NMI risk if the structure grows or the compiler changes layout assumptions. It also sits directly on the route-convergence path that was involved in the `v4.4.74` recovery failures.

## Fix

- Firmware visible version bumped to `v4.4.91`.
- `team_route_update_observe()` now copies the route-update body into a local structure before reading fields.
- `team_bind_packet_source()` now copies the route-update body once, then uses the copied local structure for next-hop decisions.
- Unit tests now reject reintroducing direct route-update body casts in the WS63 adapter.
- Version guards and runbook entry points now target `v4.4.91`.

## Verification

Completed:

- Python `py_compile`: pass.
- Python unit tests: pass, 27 tests.
- WebUI contract tests: pass, 54 tests.
- `git diff --check`: pass, CRLF normalization warnings only.
- Remote Ubuntu firmware build: pass.
- Built package: `output_from_vm/team_network_v4_unified_runtime_role/ws63-liteos-app_v4_unified_all.fwpkg`.
- Package size: `1603176`.
- Package guard: `contains_v4.4.91=True`.
- Hardware flash: not run.

Flash only after code checks and build pass, and only when explicitly continuing hardware validation.
