# Version v4.4.40

Date: 2026-06-03

## Scope

`v4.4.40` fixes the member reboot recovery path for serial-configured nodes.

The root cause found during live COM13/COM16 testing was that the shortcut serial commands `role leader` and `role member <leader_suffix>` only configured RAM. After a member reboot, the firmware printed `[team-nv] no valid web config`, so it had no saved leader suffix to restore from flash.

## What Changed

1. `role leader` now saves the leader role to NV/flash and immediately applies it.
2. `role member <leader_suffix>` now saves the member role, leader suffix, team, and channel to NV/flash and immediately applies it.
3. Board-visible firmware version is advanced to `v4.4.40` so screen and serial status can prove the new firmware is running.

## Expected Behavior

After configuring a member through the serial shortcut command, rebooting the member should load `[team-nv] restore member ...` and automatically reconnect to the saved leader without manually sending `join` or `approve` again.

## Verification

- Pre-fix live test failed: COM13 reboot printed `[team-nv] no valid web config ret=0x80003081 len=0`, and COM16 did not recover `member=241`.
- Post-fix build, flash, serial save, member reboot, and recovery test must be recorded in `MANIFEST.md` after the new firmware is built and burned.
