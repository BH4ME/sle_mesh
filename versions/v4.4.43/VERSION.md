# Version v4.4.43

Date: 2026-06-03

## Scope

`v4.4.43` keeps the `v4.4.42` link-loss/manual-leave state-machine split and adds an automation guard against false-positive live tests.

## Root Cause

The firmware-side fix already separates member reboot/signal loss from manual leave. The remaining risk was test automation: a live link-cycle test could accidentally hide a firmware recovery bug if it sent `join` or `role member` after reboot instead of waiting for the member to restore from flash/NV.

## What Changed

1. Added a link-cycle unit test that verifies the command order is `join -> reboot -> leave -> role member`.
2. The same test proves there is no `join` or `role member` command between `reboot` and `leave`.
3. The link-cycle test still requires the member reboot path to produce the NV restore log before leader online is accepted.
4. Visible firmware/build guard strings were bumped to `v4.4.43` for burn-time sanity checks.

## Expected Behavior

- Member reboot/power loss: test automation sends only `reboot`, waits for leader offline, waits for member NV restore, then waits for online.
- Member manual leave: only after `leave` and no-auto-rejoin verification does the automation send `role member <leader_suffix>` for manual rejoin.
- Relay/upstream parent loss: unchanged from `v4.4.42`; link loss preserves leader config and retries discovery.
