# Version v4.4.41

Date: 2026-06-03

## Scope

`v4.4.41` fixes the two priority member lifecycle cases:

1. A rebooted/power-cycled member is treated as signal loss, not a manual leave. The leader marks it offline by disconnect/timeout, and the member can rejoin from its saved leader configuration.
2. A manually leaving member notifies the leader without requiring leader confirmation. The leader marks it offline immediately, and the member does not auto-rejoin until it is manually configured/joined again.

## Root Cause

`sle_team_node_member_leave()` previously only cleared the member's local runtime state. It did not send any leave indication to the leader, so the leader could only notice the departure later through disconnect/heartbeat timeout. It also left the member in a discovering state, so the member could keep sending HELLO again instead of waiting for an explicit manual rejoin.

The firmware WebUI leave path cleared NV config, but the generic serial `leave` command did not. That meant WebUI and serial could behave differently after reboot.

## What Changed

1. Added `SLE_TEAM_ALERT_LEAVE`.
2. `sle_team_node_member_leave()` now best-effort sends an `ALERT_LEAVE` to the current leader before clearing local state.
3. The leader handles `ALERT_LEAVE` by marking that member offline immediately and revoking relay state.
4. Manual leave now puts the member into `SLE_TEAM_NET_IDLE` with no active leader, so it cannot silently auto-rejoin.
5. WebUI and serial `leave` now share the same firmware cleanup behavior: notify leave, clear NV, reset runtime role state, and refresh labels/display.
6. `scripts/simulate_v2.sh` now includes `examples/team_node_regression_test.c` in core/all simulation runs.

## Expected Behavior

- Member reboot/power loss: leader shows member offline first, then online again after the member boots and sends HELLO from saved config.
- Member manual leave: leader immediately sees offline; member remains idle until the operator manually reconfigures or joins.
- Manual rejoin after leave: leader accepts the HELLO and shows the member online again.

## Verification Status

Local protocol tests pass. Board build, flash, and live COM13/COM16 validation are still required because the remote Ubuntu build host currently rejects non-interactive SSH without credentials.
