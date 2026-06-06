# v4.4.40 Manifest

## Changed Files

- `xc/ws63_team_network/src/ws63_team_network_app.c`
- `README.md`
- `versions/README.md`
- `versions/v4.4.40/VERSION.md`
- `versions/v4.4.40/MANIFEST.md`

## Key Deltas

1. Serial shortcut role commands now persist configuration to NV/flash:
   - `role leader`
   - `role member <leader_suffix>`
2. Firmware display/status version changed from `v4.4.37` to `v4.4.40`.

## Live Root Cause Evidence

Before this fix, COM13 was configured as member through the shortcut CLI, but reboot recovery failed:

- `logs/auto_test/member_reboot_flash_restore_20260603_212056`
- Member boot evidence: `[team-nv] no valid web config ret=0x80003081 len=0`
- Leader result: COM16 stopped listing `member=241` and did not recover within the monitored window.

## Verification

Pending after build and burn:

- Compile v4 unified firmware on Ubuntu build host.
- Burn the new package to COM13 and COM16.
- Configure COM16 as leader and COM13 as member using the serial shortcut commands.
- Reboot COM13 only.
- Confirm COM13 logs `[team-nv] restore member leader_suffix=279A leader=154 ret=0`.
- Confirm COM16 sees `member=241 online=1` again without sending `join`.
