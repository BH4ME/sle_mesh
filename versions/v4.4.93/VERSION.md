# Version v4.4.93

Date: 2026-06-06

## Scope

`v4.4.93` replaces the fixed three-relay automatic policy with a dynamic, bounded relay budget.

The intent is to keep the leader stable while allowing the relay target to scale with the configured member table size. The temporary `cfg direct 1` setup remains useful for three-member relay tests, but normal larger deployments should use the default/direct-capacity path.

## Design

- Firmware visible version bumped to `v4.4.93`.
- Automatic relay target is now calculated from deployment demand, leader direct capacity, per-relay fanout, and a computed relay budget.
- Relay budget is bounded by:
  - `SLE_TEAM_MAX_MEMBERS`, the static member/route table capacity and main RAM cost driver.
  - `SLE_TEAM_MAX_DIRECT_CONNECTIONS`, the hardware fanout limit.
  - `SLE_TEAM_RELAY_MGMT_RAM_BUDGET_BYTES`, a management RAM budget.
  - the current leader direct cap, because current route policy keeps relay nodes directly acceptable to leader.
- Status output now exposes `runtimeRelayBudget` and route metrics expose `relayBudget`.

## Capacity Examples

- Current default: `SLE_TEAM_MAX_MEMBERS=30`, `direct_cap=8`, fanout `8` -> relay budget `3`, target for 30 members `3`.
- Three-member forced relay test: `direct_cap=1` -> relay budget `1`, enough to exercise relay election without allowing excess direct relay pressure.
- Future larger table example: if `SLE_TEAM_MAX_MEMBERS` is raised to `64` and direct cap remains `8`, the computed relay budget can grow to `7`, still capped by the management and hardware limits.

## Verification

- Python compile for changed WS63 automation files: pass.
- Python WS63 automation unit tests: pass, 52 tests.
- WebUI contract tests: pass, 58 tests.
- WebUI production build: pass.
- `git diff --check`: pass, CRLF normalization warnings only.
- Firmware compile/build: not run by user request; a previously-started local WSL build was stopped after the no-compile clarification.
- Hardware flash/burn and scaling validation: not run by user request.
